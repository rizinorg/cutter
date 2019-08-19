#include "HexWidget.h"
#include "Cutter.h"
#include "Configuration.h"

#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QtEndian>
#include <QScrollBar>
#include <QMenu>
#include <QClipboard>
#include <QApplication>

static const uint64_t MAX_COPY_SIZE = 128 * 1024 * 1024;
static const int MAX_LINE_WIDTH_PRESET = 32;
static const int MAX_LINE_WIDTH_BYTES = 128 * 1024;

HexWidget::HexWidget(QWidget *parent) :
    QScrollArea(parent),
    cursorEnabled(true),
    cursorOnAscii(false),
    updatingSelection(false),
    itemByteLen(1),
    itemGroupSize(1),
    rowSizeBytes(16),
    columnMode(ColumnMode::PowerOf2),
    itemFormat(ItemFormatHex),
    itemBigEndian(false),
    addrCharLen(AddrWidth64),
    showHeader(true),
    showAscii(true),
    showExHex(true),
    showExAddr(true)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::FocusPolicy::StrongFocus);
    connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, [this]() { viewport()->update(); });

    connect(Config(), &Configuration::colorsUpdated, this, &HexWidget::updateColors);
    connect(Config(), &Configuration::fontsUpdated, this, [this]() { setMonospaceFont(Config()->getFont()); });

    auto sizeActionGroup = new QActionGroup(this);
    for (int i = 1; i <= 8; i *= 2) {
        QAction *action = new QAction(QString::number(i), this);
        action->setCheckable(true);
        action->setActionGroup(sizeActionGroup);
        connect(action, &QAction::triggered, this, [=]() { setItemSize(i); });
        actionsItemSize.append(action);
    }
    actionsItemSize.at(0)->setChecked(true);

    /* Follow the order in ItemFormat enum */
    QStringList names;
    names << tr("Hexadecimal");
    names << tr("Octal");
    names << tr("Decimal");
    names << tr("Signed decimal");
    names << tr("Float");

    auto formatActionGroup = new QActionGroup(this);
    for (int i = 0; i < names.length(); ++i) {
        QAction *action = new QAction(names.at(i), this);
        action->setCheckable(true);
        action->setActionGroup(formatActionGroup);
        connect(action, &QAction::triggered, this, [=]() { setItemFormat(static_cast<ItemFormat>(i)); });
        actionsItemFormat.append(action);
    }
    actionsItemFormat.at(0)->setChecked(true);
    actionsItemFormat.at(ItemFormatFloat)->setEnabled(false);

    rowSizeMenu = new QMenu(tr("Bytes per row"), this);
    auto columnsActionGroup = new QActionGroup(this);
    for (int i = 1; i <= MAX_LINE_WIDTH_PRESET; i *= 2) {
        QAction *action = new QAction(QString::number(i), rowSizeMenu);
        action->setCheckable(true);
        action->setActionGroup(columnsActionGroup);
        connect(action, &QAction::triggered, this, [=]() { setFixedLineSize(i); });
        rowSizeMenu->addAction(action);
    }
    rowSizeMenu->addSeparator();
    actionRowSizePowerOf2 = new QAction(tr("Power of 2"), this);
    actionRowSizePowerOf2->setCheckable(true);
    actionRowSizePowerOf2->setActionGroup(columnsActionGroup);
    connect(actionRowSizePowerOf2, &QAction::triggered, this, [=]() { setColumnMode(ColumnMode::PowerOf2); });
    rowSizeMenu->addAction(actionRowSizePowerOf2);

    actionItemBigEndian = new QAction(tr("Big Endian"), this);
    actionItemBigEndian->setCheckable(true);
    actionItemBigEndian->setEnabled(false);
    connect(actionItemBigEndian, &QAction::triggered, this, &HexWidget::setItemEndianess);

    actionHexPairs = new QAction(tr("Bytes as pairs"), this);
    actionHexPairs->setCheckable(true);
    connect(actionHexPairs, &QAction::triggered, this, &HexWidget::onHexPairsModeEnabled);

    actionCopy = new QAction(tr("Copy"), this);
    addAction(actionCopy);
    actionCopy->setShortcutContext(Qt::ShortcutContext::WidgetWithChildrenShortcut);
    actionCopy->setShortcut(QKeySequence::Copy);
    connect(actionCopy, &QAction::triggered, this, &HexWidget::copy);

    actionCopyAddress = new QAction(tr("Copy address"), this);
    actionCopyAddress->setShortcutContext(Qt::ShortcutContext::WidgetWithChildrenShortcut);
    actionCopyAddress->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_C);
    connect(actionCopyAddress, &QAction::triggered, this, &HexWidget::copyAddress);
    addAction(actionCopyAddress);

    actionSelectRange = new QAction(tr("Select range"), this);
    connect(actionSelectRange, &QAction::triggered, this, [this]() { rangeDialog.open(cursor.address); });
    addAction(actionSelectRange);
    connect(&rangeDialog, &QDialog::accepted, this, &HexWidget::onRangeDialogAccepted);

    connect(this, &HexWidget::selectionChanged, this, [this](Selection selection) {
        actionCopy->setEnabled(!selection.empty);
    });

    updateMetrics();
    updateItemLength();

    startAddress = 0ULL;
    cursor.address = 0ULL;
    data.reset(new MemoryData());

    fetchData();
    updateCursorMeta();

    connect(&cursor.blinkTimer, &QTimer::timeout, this, &HexWidget::onCursorBlinked);
    cursor.setBlinkPeriod(1000);
    cursor.startBlinking();

    updateColors();
}

HexWidget::~HexWidget()
{

}

void HexWidget::setMonospaceFont(const QFont &font)
{
    if (!(font.styleHint() & QFont::Monospace)) {
        /* FIXME: Use default monospace font
        setFont(XXX); */
    }
    QScrollArea::setFont(font);
    monospaceFont = font.resolve(this->font());
    updateMetrics();
    fetchData();
    updateCursorMeta();

    viewport()->update();
}

void HexWidget::setItemSize(int nbytes)
{
    static const QVector<int> values({1, 2, 4, 8});

    if (!values.contains(nbytes))
        return;

    itemByteLen = nbytes;
    if (itemByteLen > rowSizeBytes) {
        rowSizeBytes = itemByteLen;
    }

    actionsItemFormat.at(ItemFormatFloat)->setEnabled(nbytes >= 4);
    actionItemBigEndian->setEnabled(nbytes != 1);

    updateItemLength();
    fetchData();
    updateCursorMeta();

    viewport()->update();
}

void HexWidget::setItemFormat(ItemFormat format)
{
    itemFormat = format;

    bool sizeEnabled = true;
    if (format == ItemFormatFloat)
        sizeEnabled = false;
    actionsItemSize.at(0)->setEnabled(sizeEnabled);
    actionsItemSize.at(1)->setEnabled(sizeEnabled);


    updateItemLength();
    fetchData();
    updateCursorMeta();

    viewport()->update();
}

void HexWidget::setItemGroupSize(int size)
{
    itemGroupSize = size;

    updateCounts();
    fetchData();
    updateCursorMeta();

    viewport()->update();
}

void HexWidget::updateCounts()
{
    actionHexPairs->setEnabled(rowSizeBytes > 1 && itemByteLen == 1
                               && itemFormat == ItemFormat::ItemFormatHex);
    actionHexPairs->setChecked(Core()->getConfigb("hex.pairs"));
    if (actionHexPairs->isChecked() && actionHexPairs->isEnabled()) {
        itemGroupSize = 2;
    } else {
        itemGroupSize = 1;
    }

    if (columnMode == ColumnMode::PowerOf2) {
        int last_good_size = itemGroupByteLen();
        for (int i = itemGroupByteLen(); i <= MAX_LINE_WIDTH_BYTES; i *= 2) {
            rowSizeBytes = i;
            itemColumns = rowSizeBytes / itemGroupByteLen();
            updateAreasPosition();
            if (horizontalScrollBar()->maximum() == 0) {
                last_good_size = rowSizeBytes;
            } else {
                break;
            }
        }
        rowSizeBytes = last_good_size;
    }

    itemColumns = rowSizeBytes / itemGroupByteLen();

    // ensure correct action is selected when changing line size programmatically
    if (columnMode == ColumnMode::Fixed) {
        int w = 1;
        const auto &actions = rowSizeMenu->actions();
        for (auto action : actions) {
            action->setChecked(false);
        }
        for (auto action : actions) {
            if (w > MAX_LINE_WIDTH_PRESET) {
                break;
            }
            if (rowSizeBytes == w) {
                action->setChecked(true);
            }
            w *= 2;
        }
    } else if (columnMode == ColumnMode::PowerOf2) {
        actionRowSizePowerOf2->setChecked(true);
    }

    updateAreasPosition();
}

void HexWidget::setFixedLineSize(int lineSize)
{
    if (lineSize < 1 || lineSize < itemGroupByteLen() || lineSize % itemGroupByteLen()) {
        updateCounts();
        return;
    }
    rowSizeBytes = lineSize;
    columnMode = ColumnMode::Fixed;

    updateCounts();
    fetchData();
    updateCursorMeta();

    viewport()->update();
}

void HexWidget::setColumnMode(ColumnMode mode)
{
    columnMode = mode;

    updateCounts();
    fetchData();
    updateCursorMeta();

    viewport()->update();
}

void HexWidget::selectRange(RVA start, RVA end)
{
    BasicCursor endCursor(end);
    endCursor += 1;
    setCursorAddr(endCursor);
    selection.set(start, end);
    cursorEnabled = false;
    emit selectionChanged(getSelection());
}

void HexWidget::clearSelection()
{
    setCursorAddr(cursor.address, false);
    emit selectionChanged(getSelection());
}

HexWidget::Selection HexWidget::getSelection()
{
    return Selection{selection.isEmpty(), selection.start(), selection.end()};
}

void HexWidget::seek(uint64_t address)
{
    setCursorAddr(address);
}

void HexWidget::refresh()
{
    fetchData();
    viewport()->update();
}

void HexWidget::setItemEndianess(bool bigEndian)
{
    itemBigEndian = bigEndian;

    updateCursorMeta(); // Update cached item character

    viewport()->update();
}

void HexWidget::updateColors()
{
    borderColor = Config()->getColor("gui.border");
    backgroundColor = Config()->getColor("gui.background");
    b0x00Color = Config()->getColor("b0x00");
    b0x7fColor = Config()->getColor("b0x7f");
    b0xffColor = Config()->getColor("b0xff");
    printableColor = Config()->getColor("ai.write");
    defColor = Config()->getColor("btext");
    addrColor = Config()->getColor("func_var_addr");

    updateCursorMeta();
    viewport()->update();
}

void HexWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(viewport());
    painter.setFont(monospaceFont);

    int xOffset = horizontalScrollBar()->value();
    if (xOffset > 0)
        painter.translate(QPoint(-xOffset, 0));

    if (event->rect() == cursor.screenPos) {
        /* Cursor blink */
        drawCursor(painter);
        return;
    }

    painter.fillRect(event->rect().translated(xOffset, 0), backgroundColor);

    drawHeader(painter);

    drawAddrArea(painter);
    drawItemArea(painter);
    drawAsciiArea(painter);

    if (!cursorEnabled)
        return;

    drawCursor(painter, true);
}

void HexWidget::updateWidth()
{
    int max = (showAscii ? asciiArea.right() : itemArea.right()) - viewport()->width();
    if (max < 0)
        max = 0;
    else
        max += charWidth;
    horizontalScrollBar()->setMaximum(max);
    horizontalScrollBar()->setSingleStep(charWidth);
}

void HexWidget::resizeEvent(QResizeEvent *event)
{
    int oldByteCount = bytesPerScreen();
    updateCounts();

    if (event->oldSize().height() == event->size().height() && oldByteCount == bytesPerScreen())
        return;

    updateAreasHeight();
    fetchData(); // rowCount was changed
    updateCursorMeta();

    viewport()->update();
}

void HexWidget::mouseMoveEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();
    pos.rx() += horizontalScrollBar()->value();

    if (!updatingSelection) {
        if (itemArea.contains(pos) || asciiArea.contains(pos))
            setCursor(Qt::IBeamCursor);
        else
            setCursor(Qt::ArrowCursor);
        return;
    }

    auto &area = currentArea();
    if (pos.x() < area.left())
        pos.setX(area.left());
    else if (pos.x() > area.right())
        pos.setX(area.right());
    auto addr = currentAreaPosToAddr(pos, true);
    setCursorAddr(addr, true);

    /* Stop blinking */
    cursorEnabled = false;

    viewport()->update();
}

void HexWidget::mousePressEvent(QMouseEvent *event)
{
    QPoint pos(event->pos());
    pos.rx() += horizontalScrollBar()->value();

    if (event->button() == Qt::LeftButton) {
        bool selectingData = itemArea.contains(pos);
        bool selecting = selectingData || asciiArea.contains(pos);
        if (selecting) {
            updatingSelection = true;
            setCursorOnAscii(!selectingData);
            auto cursorPosition = currentAreaPosToAddr(pos, true);
            setCursorAddr(cursorPosition, event->modifiers() == Qt::ShiftModifier);
            viewport()->update();
        }
    }
}

void HexWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (selection.isEmpty()) {
            selection.init(cursor.address);
            cursorEnabled = true;
        }
        updatingSelection = false;
    }
}

void HexWidget::wheelEvent(QWheelEvent *event)
{
    int dy = event->delta();
    int64_t delta = 3 * itemRowByteLen();
    if (dy > 0)
        delta = -delta;

    if (dy == 0)
        return;

    if (delta < 0 && startAddress < static_cast<uint64_t>(-delta)) {
        startAddress = 0;
    } else if (delta > 0 && data->maxIndex() < static_cast<uint64_t>(bytesPerScreen())) {
        startAddress = 0;
    } else if (delta > 0
               && (data->maxIndex() - startAddress) <= static_cast<uint64_t>(bytesPerScreen() + delta - 1)) {
        startAddress = (data->maxIndex() - bytesPerScreen()) + 1;
    } else {
        startAddress += delta;
    }
    fetchData();
    if (cursor.address >= startAddress && cursor.address <= lastVisibleAddr()) {
        /* Don't enable cursor blinking if selection isn't empty */
        cursorEnabled = selection.isEmpty();
        updateCursorMeta();
    } else {
        cursorEnabled = false;
    }
    viewport()->update();
}

void HexWidget::keyPressEvent(QKeyEvent *event)
{
    bool select = false;
    auto moveOrSelect = [event, &select](QKeySequence::StandardKey moveSeq, QKeySequence::StandardKey selectSeq) ->bool {
        if (event->matches(moveSeq)) {
            select = false;
            return true;
        } else if (event->matches(selectSeq)) {
            select = true;
            return true;
        }
        return false;
    };
    if (moveOrSelect(QKeySequence::MoveToNextLine, QKeySequence::SelectNextLine)) {
        moveCursor(itemRowByteLen(), select);
    } else if (moveOrSelect(QKeySequence::MoveToPreviousLine, QKeySequence::SelectPreviousLine)) {
        moveCursor(-itemRowByteLen(), select);
    } else if (moveOrSelect(QKeySequence::MoveToNextChar, QKeySequence::SelectNextChar)) {
        moveCursor(cursorOnAscii ? 1 : itemByteLen, select);
    } else if (moveOrSelect(QKeySequence::MoveToPreviousChar, QKeySequence::SelectPreviousChar)) {
        moveCursor(cursorOnAscii ? -1 : -itemByteLen, select);
    } else if (moveOrSelect(QKeySequence::MoveToNextPage, QKeySequence::SelectNextPage)) {
        moveCursor(bytesPerScreen(), select);
    } else if (moveOrSelect(QKeySequence::MoveToPreviousPage, QKeySequence::SelectPreviousPage)) {
        moveCursor(-bytesPerScreen(), select);
    } else if (moveOrSelect(QKeySequence::MoveToStartOfLine, QKeySequence::SelectStartOfLine)) {
        int linePos = int((cursor.address % itemRowByteLen()) - (startAddress % itemRowByteLen()));
        moveCursor(-linePos, select);
    } else if (moveOrSelect(QKeySequence::MoveToEndOfLine, QKeySequence::SelectEndOfLine)) {
        int linePos = int((cursor.address % itemRowByteLen()) - (startAddress % itemRowByteLen()));
        moveCursor(itemRowByteLen() - linePos, select);
    }
    //viewport()->update();
}

void HexWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QPoint pt = event->pos();
    bool mouseOutsideSelection = false;
    if (event->reason() == QContextMenuEvent::Mouse) {
        auto mouseAddr = mousePosToAddr(pt).address;
        if (asciiArea.contains(pt)) {
            cursorOnAscii = true;
        } else if (itemArea.contains(pt)) {
            cursorOnAscii = false;
        }
        if (selection.isEmpty()) {
            seek(mouseAddr);
        } else {
            mouseOutsideSelection = !selection.contains(mouseAddr);
        }
    }

    auto disableOutsideSelectionActions = [this](bool disable) {
        actionCopyAddress->setDisabled(disable);
    };

    QMenu *menu = new QMenu();
    QMenu *sizeMenu = menu->addMenu(tr("Item size:"));
    sizeMenu->addActions(actionsItemSize);
    QMenu *formatMenu = menu->addMenu(tr("Item format:"));
    formatMenu->addActions(actionsItemFormat);
    menu->addMenu(rowSizeMenu);
    menu->addAction(actionHexPairs);
    menu->addAction(actionItemBigEndian);
    menu->addSeparator();
    menu->addAction(actionCopy);
    disableOutsideSelectionActions(mouseOutsideSelection);
    menu->addAction(actionCopyAddress);
    menu->addActions(this->actions());
    menu->exec(mapToGlobal(pt));
    disableOutsideSelectionActions(false);
    menu->deleteLater();
}

void HexWidget::onCursorBlinked()
{
    if (!cursorEnabled)
        return;
    cursor.blink();
    QRect cursorRect(cursor.screenPos.x(), cursor.screenPos.y(), cursor.screenPos.width(), cursor.screenPos.height());
    viewport()->update(cursorRect.translated(-horizontalScrollBar()->value(), 0));
}

void HexWidget::onHexPairsModeEnabled(bool enable)
{
    // Sync configuration
    Core()->setConfig("hex.pairs", enable);
    if (enable) {
        setItemGroupSize(2);
    } else {
        setItemGroupSize(1);
    }
}

void HexWidget::copy()
{
    if (selection.isEmpty() || selection.size() > MAX_COPY_SIZE)
        return;

    QClipboard *clipboard = QApplication::clipboard();
    QString range = QString("%1@0x%2").arg(selection.size()).arg(selection.start(), 0, 16);
    if (cursorOnAscii) {
        clipboard->setText(Core()->cmd("psx " + range));
    } else {
        clipboard->setText(Core()->cmd("p8 " + range)); //TODO: copy in the format shown
    }
}

void HexWidget::copyAddress()
{
    uint64_t addr = cursor.address;
    if (!selection.isEmpty()) {
        addr = selection.start();
    }
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(RAddressString(addr));
}

void HexWidget::onRangeDialogAccepted()
{
    if (rangeDialog.empty()) {
        seek(rangeDialog.getStartAddress());
        return;
    }
    selectRange(rangeDialog.getStartAddress(), rangeDialog.getEndAddress());
}

void HexWidget::updateItemLength()
{
    itemPrefixLen = 0;

    switch (itemFormat) {
    case ItemFormatHex:
        itemCharLen = 2 * itemByteLen;
        if (itemByteLen > 1 && showExHex)
            itemPrefixLen = hexPrefix.length();
        break;
    case ItemFormatOct:
        itemCharLen = (itemByteLen * 8 + 3) / 3;
        break;
    case ItemFormatDec:
        switch (itemByteLen) {
        case 1:
            itemCharLen = 3;
            break;
        case 2:
            itemCharLen = 5;
            break;
        case 4:
            itemCharLen = 10;
            break;
        case 8:
            itemCharLen = 20;
            break;
        }
        break;
    case ItemFormatSignedDec:
        switch (itemByteLen) {
        case 1:
            itemCharLen = 4;
            break;
        case 2:
            itemCharLen = 6;
            break;
        case 4:
            itemCharLen = 11;
            break;
        case 8:
            itemCharLen = 20;
            break;
        }
        break;
    case ItemFormatFloat:
        if (itemByteLen < 4)
            itemByteLen = 4;
        // FIXME
        itemCharLen = 3 * itemByteLen;
        break;
    }

    itemCharLen += itemPrefixLen;

    updateCounts();
}

void HexWidget::drawHeader(QPainter &painter)
{
    if (!showHeader)
        return;

    int offset = 0;
    QRectF rect(itemArea.left(), 0, itemWidth(), lineHeight);

    painter.setPen(addrColor);

    for (int j = 0; j < itemColumns; ++j) {
        for (int k = 0; k < itemGroupSize; ++k, offset += itemByteLen) {
            painter.drawText(rect, Qt::AlignVCenter | Qt::AlignRight, QString::number(offset, 16).toUpper());
            rect.translate(itemWidth(), 0);
        }
        rect.translate(columnSpacingWidth(), 0);
    }

    rect.moveLeft(asciiArea.left());
    rect.setWidth(charWidth);
    for (int j = 0; j < itemRowByteLen(); ++j) {
        painter.drawText(rect, Qt::AlignVCenter | Qt::AlignRight, QString::number(j % 16, 16).toUpper());
        rect.translate(charWidth, 0);
    }
}

void HexWidget::drawCursor(QPainter &painter, bool shadow)
{
    if (shadow) {
        QPen pen(Qt::gray);
        pen.setStyle(Qt::DashLine);
        painter.setPen(pen);
        shadowCursor.screenPos.setWidth(cursorOnAscii ? itemWidth() : charWidth);
        painter.drawRect(shadowCursor.screenPos);
        painter.setPen(Qt::SolidLine);
    }

    painter.setPen(cursor.cachedColor);
    QRectF charRect(cursor.screenPos);
    charRect.setWidth(charWidth);
    painter.fillRect(charRect, backgroundColor);
    painter.drawText(charRect, Qt::AlignVCenter, cursor.cachedChar);
    if (cursor.isVisible) {
        painter.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
        painter.fillRect(cursor.screenPos, QColor(0xff, 0xff, 0xff));
    }
}

void HexWidget::drawAddrArea(QPainter &painter)
{
    uint64_t offset = startAddress;
    QString addrString;
    QSizeF areaSize((addrCharLen + (showExAddr ? 2 : 0)) * charWidth, lineHeight);
    QRectF strRect(addrArea.topLeft(), areaSize);

    painter.setPen(addrColor);
    for (int line = 0;
            line < visibleLines && offset <= data->maxIndex();
            ++line, strRect.translate(0, lineHeight), offset += itemRowByteLen()) {
        addrString = QString("%1").arg(offset, addrCharLen, 16, QLatin1Char('0'));
        if (showExAddr)
            addrString.prepend(hexPrefix);
        painter.drawText(strRect, Qt::AlignVCenter, addrString);
    }

    painter.setPen(borderColor);

    qreal vLineOffset = itemArea.left() - charWidth;
    painter.drawLine(QLineF(vLineOffset, 0, vLineOffset, viewport()->height()));
}

void HexWidget::drawItemArea(QPainter &painter)
{
    QRectF itemRect(itemArea.topLeft(), QSizeF(itemWidth(), lineHeight));
    QColor itemColor;
    QString itemString;

    fillSelectionBackground(painter);

    uint64_t itemAddr = startAddress;
    for (int line = 0; line < visibleLines; ++line) {
        itemRect.moveLeft(itemArea.left());
        for (int j = 0; j < itemColumns; ++j) {
            for (int k = 0; k < itemGroupSize && itemAddr <= data->maxIndex(); ++k, itemAddr += itemByteLen) {
                itemString = renderItem(itemAddr - startAddress, &itemColor);
                if (selection.contains(itemAddr)  && !cursorOnAscii) {
                    itemColor = palette().highlightedText().color();
                }
                painter.setPen(itemColor);
                painter.drawText(itemRect, Qt::AlignVCenter, itemString);
                itemRect.translate(itemWidth(), 0);
                if (cursor.address == itemAddr) {
                    auto &itemCursor = cursorOnAscii ? shadowCursor : cursor;
                    itemCursor.cachedChar = itemString.at(0);
                    itemCursor.cachedColor = itemColor;
                }
            }
            itemRect.translate(columnSpacingWidth(), 0);
        }
        itemRect.translate(0, lineHeight);
    }

    painter.setPen(borderColor);

    qreal vLineOffset = asciiArea.left() - charWidth;
    painter.drawLine(QLineF(vLineOffset, 0, vLineOffset, viewport()->height()));
}

void HexWidget::drawAsciiArea(QPainter &painter)
{
    QRectF charRect(asciiArea.topLeft(), QSizeF(charWidth, lineHeight));

    fillSelectionBackground(painter, true);

    uint64_t address = startAddress;
    QChar ascii;
    QColor color;
    for (int line = 0; line < visibleLines; ++line, charRect.translate(0, lineHeight)) {
        charRect.moveLeft(asciiArea.left());
        for (int j = 0; j < itemRowByteLen() && address <= data->maxIndex(); ++j, ++address) {
            ascii = renderAscii(address - startAddress, &color);
            if (selection.contains(address) && cursorOnAscii)
                color = palette().highlightedText().color();
            painter.setPen(color);
            /* Dots look ugly. Use fillRect() instead of drawText(). */
            if (ascii == '.') {
                qreal a = cursor.screenPos.width();
                QPointF p = charRect.bottomLeft();
                p.rx() += (charWidth - a) / 2 + 1;
                p.ry() += - 2 * a;
                painter.fillRect(QRectF(p, QSizeF(a, a)), color);
            } else {
                painter.drawText(charRect, Qt::AlignVCenter, ascii);
            }
            charRect.translate(charWidth, 0);
            if (cursor.address == address) {
                auto &itemCursor = cursorOnAscii ? cursor : shadowCursor;
                itemCursor.cachedChar = ascii;
                itemCursor.cachedColor = color;
            }
        }
    }
}

void HexWidget::fillSelectionBackground(QPainter &painter, bool ascii)
{
    if (selection.isEmpty()) {
        return;
    }
    const auto parts = rangePolygons(selection.start(), selection.end(), ascii);
    for (const auto &shape : qAsConst(parts)) {
        QColor highlightColor = palette().color(QPalette::Highlight);
        if (ascii == cursorOnAscii) {
            painter.setBrush(highlightColor);
            painter.drawPolygon(shape);
        } else {
            painter.setPen(highlightColor);
            painter.drawPolyline(shape);
        }
    }
}

QVector<QPolygonF> HexWidget::rangePolygons(RVA start, RVA last, bool ascii)
{
    if (last < startAddress || start > lastVisibleAddr()) {
        return {};
    }

    QRectF rect;
    const QRectF area = QRectF(ascii ? asciiArea : itemArea);

    /* Convert absolute values to relative */
    int startOffset = std::max(uint64_t(start), startAddress) - startAddress;
    int endOffset = std::min(uint64_t(last), lastVisibleAddr()) - startAddress;

    QVector<QPolygonF> parts;

    auto getRectangle = [&](int offset) {
        return QRectF(ascii ? asciiRectangle(offset) : itemRectangle(offset));
    };

    auto startRect = getRectangle(startOffset);
    auto endRect = getRectangle(endOffset);
    if (!ascii) {
        if (int startFraction = startOffset % itemByteLen) {
            startRect.setLeft(startRect.left() + startFraction * startRect.width() / itemByteLen);
        }
        if (int endFraction = itemByteLen - 1 - (endOffset % itemByteLen)) {
            endRect.setRight(endRect.right() - endFraction * endRect.width() / itemByteLen);
        }
    }
    if (endOffset - startOffset + 1 <= rowSizeBytes) {
        if (startOffset / rowSizeBytes == endOffset / rowSizeBytes) { // single row
            rect = startRect;
            rect.setRight(endRect.right());
            parts.push_back(QPolygonF(rect));
        } else {
            // two seperate rectangles
            rect = startRect;
            rect.setRight(area.right());
            parts.push_back(QPolygonF(rect));
            rect = endRect;
            rect.setLeft(area.left());
            parts.push_back(QPolygonF(rect));
        }
    } else {
        // single multiline shape
        QPolygonF shape;
        shape << startRect.topLeft();
        rect = getRectangle(startOffset + rowSizeBytes - 1 - startOffset % rowSizeBytes);
        shape << rect.topRight();
        if (endOffset % rowSizeBytes != rowSizeBytes - 1) {
            rect = getRectangle(endOffset - endOffset % rowSizeBytes - 1);
            shape << rect.bottomRight() << endRect.topRight();
        }
        shape << endRect.bottomRight();
        shape << getRectangle(endOffset - endOffset % rowSizeBytes).bottomLeft();
        if (startOffset % rowSizeBytes) {
            rect = getRectangle(startOffset - startOffset % rowSizeBytes + rowSizeBytes);
            shape << rect.topLeft() << startRect.bottomLeft();
        }
        shape << shape.first(); // close the shape
        parts.push_back(shape);
    }
    return parts;
}

void HexWidget::updateMetrics()
{
    QFontMetricsF fontMetrics(this->monospaceFont);
    lineHeight = fontMetrics.height();
    charWidth = fontMetrics.width(QLatin1Char('F'));

    updateCounts();
    updateAreasHeight();

    qreal cursorWidth = std::max(charWidth / 3, 1.);
    cursor.screenPos.setHeight(lineHeight);
    shadowCursor.screenPos.setHeight(lineHeight);

    cursor.screenPos.setWidth(cursorWidth);
    if (cursorOnAscii) {
        cursor.screenPos.moveTopLeft(asciiArea.topLeft());

        shadowCursor.screenPos.setWidth(itemWidth());
        shadowCursor.screenPos.moveTopLeft(itemArea.topLeft());
    } else {
        cursor.screenPos.moveTopLeft(itemArea.topLeft());
        shadowCursor.screenPos.setWidth(charWidth);
        shadowCursor.screenPos.moveTopLeft(asciiArea.topLeft());
    }
}

void HexWidget::updateAreasPosition()
{
    const qreal spacingWidth = areaSpacingWidth();

    qreal yOffset = showHeader ? lineHeight : 0;

    addrArea.setTopLeft(QPointF(0, yOffset));
    addrArea.setWidth((addrCharLen + (showExAddr ? 2 : 0)) * charWidth);

    itemArea.setTopLeft(QPointF(addrArea.right() + spacingWidth, yOffset));
    itemArea.setWidth(itemRowWidth());

    asciiArea.setTopLeft(QPointF(itemArea.right() + spacingWidth, yOffset));
    asciiArea.setWidth(asciiRowWidth());

    updateWidth();
}

void HexWidget::updateAreasHeight()
{
    visibleLines = static_cast<int>((viewport()->height() - itemArea.top()) / lineHeight);

    qreal height = visibleLines * lineHeight;
    addrArea.setHeight(height);
    itemArea.setHeight(height);
    asciiArea.setHeight(height);
}

void HexWidget::moveCursor(int offset, bool select)
{
    BasicCursor addr = cursor.address;
    addr += offset;
    if (addr.address > data->maxIndex()) {
        addr.address = data->maxIndex();
    }
    setCursorAddr(addr, select);
}

void HexWidget::setCursorAddr(BasicCursor addr, bool select)
{
    if (!select) {
        bool clearingSelection = !selection.isEmpty();
        selection.init(addr);
        if (clearingSelection)
            emit selectionChanged(getSelection());
    }
    emit positionChanged(addr.address);

    cursor.address = addr.address;

    /* Pause cursor repainting */
    cursorEnabled = false;

    if (select) {
        selection.update(addr);
        emit selectionChanged(getSelection());
    }

    uint64_t addressValue = cursor.address;
    /* Update data cache if necessary */
    if (!(addressValue >= startAddress && addressValue <= lastVisibleAddr())) {
        /* Align start address */
        addressValue -= (addressValue % itemRowByteLen());

        if (addressValue > (data->maxIndex() - bytesPerScreen()) + 1) {
            addressValue = (data->maxIndex() - bytesPerScreen()) + 1;
        }

        /* FIXME: handling Page Up/Down */
        if (addressValue == startAddress + bytesPerScreen()) {
            startAddress += itemRowByteLen();
        } else {
            startAddress = addressValue;
        }

        fetchData();
    }

    updateCursorMeta();

    /* Draw cursor */
    cursor.isVisible = !select;
    viewport()->update();

    /* Resume cursor repainting */
    cursorEnabled = selection.isEmpty();
}

void HexWidget::updateCursorMeta()
{
    QPointF point;
    QPointF pointAscii;

    int offset = cursor.address - startAddress;
    int itemOffset = offset;
    int asciiOffset;

    /* Calc common Y coordinate */
    point.ry() = (itemOffset / itemRowByteLen()) * lineHeight;
    pointAscii.setY(point.y());
    itemOffset %= itemRowByteLen();
    asciiOffset = itemOffset;

    /* Calc X coordinate on the item area */
    point.rx() = (itemOffset / itemGroupByteLen()) * columnExWidth();
    itemOffset %= itemGroupByteLen();
    point.rx() += (itemOffset / itemByteLen) * itemWidth();

    /* Calc X coordinate on the ascii area */
    pointAscii.rx() = asciiOffset * charWidth;

    point += itemArea.topLeft();
    pointAscii += asciiArea.topLeft();

    cursor.screenPos.moveTopLeft(cursorOnAscii ? pointAscii : point);
    shadowCursor.screenPos.moveTopLeft(cursorOnAscii ? point : pointAscii);
}

void HexWidget::setCursorOnAscii(bool ascii)
{
    cursorOnAscii = ascii;
}

const QColor HexWidget::itemColor(uint8_t byte)
{
    QColor color(defColor);

    if (byte == 0x00)
        color = b0x00Color;
    else if (byte == 0x7f)
        color = b0x7fColor;
    else if (byte == 0xff)
        color = b0xffColor;
    else if (IS_PRINTABLE(byte)) {
        color = printableColor;
    }

    return color;
}

QVariant HexWidget::readItem(int offset, QColor *color)
{
    quint8 byte;
    quint16 word;
    quint32 dword;
    quint64 qword;
    float *ptrFloat32;
    double *ptrFloat64;

    const void *dataPtr = data->dataPtr(startAddress + offset);
    const bool signedItem = itemFormat == ItemFormatSignedDec;

    switch (itemByteLen) {
    case 1:
        byte = *static_cast<const quint8 *>(dataPtr);
        if (color)
            *color = itemColor(byte);
        if (!signedItem)
            return QVariant(static_cast<quint64>(byte));
        return QVariant(static_cast<qint64>(static_cast<qint8>(byte)));
    case 2:
        if (itemBigEndian)
            word = qFromBigEndian<quint16>(dataPtr);
        else
            word = qFromLittleEndian<quint16>(dataPtr);
        if (color)
            *color = defColor;
        if (!signedItem)
            return QVariant(static_cast<quint64>(word));
        return QVariant(static_cast<qint64>(static_cast<qint16>(word)));
    case 4:
        if (itemBigEndian)
            dword = qFromBigEndian<quint32>(dataPtr);
        else
            dword = qFromLittleEndian<quint32>(dataPtr);
        if (color)
            *color = defColor;
        if (itemFormat == ItemFormatFloat) {
            ptrFloat32 = static_cast<float *>(static_cast<void *>(&dword));
            return QVariant(*ptrFloat32);
        }
        if (!signedItem)
            return QVariant(static_cast<quint64>(dword));
        return QVariant(static_cast<qint64>(static_cast<qint32>(dword)));
    case 8:
        if (itemBigEndian)
            qword = qFromBigEndian<quint64>(dataPtr);
        else
            qword = qFromLittleEndian<quint64>(dataPtr);
        if (color)
            *color = defColor;
        if (itemFormat == ItemFormatFloat) {
            ptrFloat64 = static_cast<double *>(static_cast<void *>(&qword));
            return  QVariant(*ptrFloat64);
        }
        if (!signedItem)
            return  QVariant(qword);
        return QVariant(static_cast<qint64>(qword));
    }

    return QVariant();
}

QString HexWidget::renderItem(int offset, QColor *color)
{
    QString item;
    QVariant itemVal = readItem(offset, color);
    int itemLen = itemCharLen - itemPrefixLen; /* Reserve space for prefix */

    //FIXME: handle broken itemVal ( QVariant() )
    switch (itemFormat) {
    case ItemFormatHex:
        item = QString("%1").arg(itemVal.toULongLong(), itemLen, 16, QLatin1Char('0'));
        if (itemByteLen > 1 && showExHex)
            item.prepend(hexPrefix);
        break;
    case ItemFormatOct:
        item = QString("%1").arg(itemVal.toULongLong(), itemLen, 8, QLatin1Char('0'));
        break;
    case ItemFormatDec:
        item = QString("%1").arg(itemVal.toULongLong(), itemLen, 10);
        break;
    case ItemFormatSignedDec:
        item = QString("%1").arg(itemVal.toLongLong(), itemLen, 10);
        break;
    case ItemFormatFloat:
        item = QString("%1").arg(itemVal.toDouble(), itemLen);
        break;
    }

    return item;
}

QChar HexWidget::renderAscii(int offset, QColor *color)
{
    uchar byte = *static_cast<const uint8_t *>(data->dataPtr(startAddress + offset));
    if (color) {
        *color = itemColor(byte);
    }
    if (!IS_PRINTABLE(byte)) {
        byte = '.';
    }
    return QChar(byte);
}

void HexWidget::fetchData()
{
    data->fetch(startAddress, bytesPerScreen());
}

BasicCursor HexWidget::screenPosToAddr(const QPoint &point,  bool middle) const
{
    QPointF pt = point - itemArea.topLeft();

    int relativeAddress = 0;
    int line = static_cast<int>(pt.y() / lineHeight);
    relativeAddress += line * itemRowByteLen();
    int column = static_cast<int>(pt.x() / columnExWidth());
    relativeAddress += column * itemGroupByteLen();
    pt.rx() -= column * columnExWidth();
    auto roundingOffset = middle ? itemWidth() / 2 : 0;
    relativeAddress += static_cast<int>((pt.x() + roundingOffset) / itemWidth()) * itemByteLen;
    BasicCursor result(startAddress);
    result += relativeAddress;
    return result;
}

BasicCursor HexWidget::asciiPosToAddr(const QPoint &point, bool middle) const
{
    QPointF pt = point - asciiArea.topLeft();

    int relativeAddress = 0;
    relativeAddress += static_cast<int>(pt.y() / lineHeight) * itemRowByteLen();
    auto roundingOffset = middle ? (charWidth / 2) : 0;
    relativeAddress += static_cast<int>((pt.x() + (roundingOffset)) / charWidth);
    BasicCursor result(startAddress);
    result += relativeAddress;
    return result;
}

BasicCursor HexWidget::currentAreaPosToAddr(const QPoint &point, bool middle) const
{
    return cursorOnAscii ? asciiPosToAddr(point, middle) : screenPosToAddr(point, middle);
}

BasicCursor HexWidget::mousePosToAddr(const QPoint &point, bool middle) const
{
    return asciiArea.contains(point) ? asciiPosToAddr(point, middle) : screenPosToAddr(point, middle);
}

QRectF HexWidget::itemRectangle(uint offset)
{
    qreal x;
    qreal y;

    qreal width = itemWidth();
    y = (offset / itemRowByteLen()) * lineHeight;
    offset %= itemRowByteLen();

    x = (offset / itemGroupByteLen()) * columnExWidth();
    offset %= itemGroupByteLen();
    x += (offset / itemByteLen) * itemWidth();
    if (offset == 0) {
        x -= charWidth / 2;
        width += charWidth / 2;
    }
    if (static_cast<int>(offset) == itemGroupByteLen() - 1) {
        width += charWidth / 2;
    }

    x += itemArea.x();
    y += itemArea.y();

    return QRectF(x, y, width, lineHeight);
}

QRectF HexWidget::asciiRectangle(uint offset)
{
    QPointF p;

    p.ry() = (offset / itemRowByteLen()) * lineHeight;
    offset %= itemRowByteLen();

    p.rx() = offset * charWidth;

    p += asciiArea.topLeft();

    return QRectF(p, QSizeF(charWidth, lineHeight));
}
