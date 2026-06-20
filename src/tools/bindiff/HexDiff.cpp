#include "HexDiff.h"

#include "Configuration.h"
#include "Cutter.h"
#include "dialogs/CommentsDialog.h"
#include "dialogs/FlagDialog.h"
#include "dialogs/MarkDialog.h"
#include "dialogs/WriteCommandsDialogs.h"
#include "shortcuts/ShortcutManager.h"
#include "widgets/AddressRangeScrollBar.h"

#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonObject>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPushButton>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QScrollBar>
#include <QToolTip>
#include <QWheelEvent>
#include <QtEndian>

namespace {
constexpr uint64_t maxCopySize = 128 * 1024 * 1024;
constexpr int maxLineWidthPreset = 32;
constexpr int maxLineWidthBytes = 128 * 1024;
constexpr int warningTimeMs = 500;
}

HexDiff::HexDiff(QWidget *parent)
    : QScrollArea(parent),
      cursorEnabled(true),
      cursorArea(DiffArea::ItemA),
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
      showExAddr(true),
      warningTimer(this),
      vScrollBar(new AddressRangeScrollBar(this))
{
    setMouseTracking(true);
    setFocusPolicy(Qt::FocusPolicy::StrongFocus);
    connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, &HexDiff::updateViewport);

    connect(Config(), &Configuration::colorsUpdated, this, &HexDiff::updateColors);
    connect(Config(), &Configuration::fontsUpdated, this,
            [this]() { setMonospaceFont(Config()->getFont()); });

    setVerticalScrollBar(vScrollBar);
    vScrollBar->setPageStep(10);
    vScrollBar->setSingleStep(1);
    connect(vScrollBar, &AddressRangeScrollBar::scrolled, this,
            [this](int lines) { scrollLines(lines, true); });
    connect(vScrollBar, &QScrollBar::valueChanged, this,
            [this](int) { setStartAddress(vScrollBar->address()); });
    connect(vScrollBar, &AddressRangeScrollBar::hideScrollBar, this,
            [this]() { setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); });
    connect(vScrollBar, &AddressRangeScrollBar::showScrollBar, this,
            [this]() { setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn); });
    vScrollBar->refreshRange();

    auto sizeActionGroup = new QActionGroup(this);
    for (int i = 1; i <= 8; i *= 2) {
        auto *action = new QAction(QString::number(i), this);
        action->setCheckable(true);
        action->setActionGroup(sizeActionGroup);
        connect(action, &QAction::triggered, this, [=, this]() { setItemSize(i); });
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
        auto *action = new QAction(names.at(i), this);
        action->setCheckable(true);
        action->setActionGroup(formatActionGroup);
        connect(action, &QAction::triggered, this,
                [=, this]() { setItemFormat(static_cast<ItemFormat>(i)); });
        actionsItemFormat.append(action);
    }
    actionsItemFormat.at(0)->setChecked(true);
    actionsItemFormat.at(ItemFormatFloat)->setEnabled(false);

    rowSizeMenu = new QMenu(tr("Bytes per row"), this);
    auto columnsActionGroup = new QActionGroup(this);
    for (int i = 1; i <= maxLineWidthPreset; i *= 2) {
        auto *action = new QAction(QString::number(i), rowSizeMenu);
        action->setCheckable(true);
        action->setActionGroup(columnsActionGroup);
        connect(action, &QAction::triggered, this, [=, this]() { setFixedLineSize(i); });
        rowSizeMenu->addAction(action);
    }
    rowSizeMenu->addSeparator();
    actionRowSizePowerOf2 = new QAction(tr("Power of 2"), this);
    actionRowSizePowerOf2->setCheckable(true);
    actionRowSizePowerOf2->setActionGroup(columnsActionGroup);
    connect(actionRowSizePowerOf2, &QAction::triggered, this,
            [=, this]() { setColumnMode(ColumnMode::PowerOf2); });
    rowSizeMenu->addAction(actionRowSizePowerOf2);

    actionItemBigEndian = new QAction(tr("Big Endian"), this);
    actionItemBigEndian->setCheckable(true);
    actionItemBigEndian->setEnabled(false);
    connect(actionItemBigEndian, &QAction::triggered, this, &HexDiff::setItemEndianness);

    actionHexPairs = new QAction(tr("Bytes as pairs"), this);
    actionHexPairs->setCheckable(true);
    connect(actionHexPairs, &QAction::triggered, this, &HexDiff::onHexPairsModeEnabled);

    actionCopy = Shortcuts()->makeAction("Hex.copy", this);
    addAction(actionCopy);
    actionCopy->setShortcutContext(Qt::ShortcutContext::WidgetWithChildrenShortcut);
    connect(actionCopy, &QAction::triggered, this, &HexDiff::copy);

    actionCopyAddress = Shortcuts()->makeAction("General.copyAddress", this);
    actionCopyAddress->setShortcutContext(Qt::ShortcutContext::WidgetWithChildrenShortcut);
    connect(actionCopyAddress, &QAction::triggered, this, &HexDiff::copyAddress);
    addAction(actionCopyAddress);

    actionSelectRange = new QAction(tr("Select range"), this);
    connect(actionSelectRange, &QAction::triggered, this,
            [this]() { rangeDialog.openAt(cursor.address); });
    addAction(actionSelectRange);
    connect(&rangeDialog, &QDialog::accepted, this, &HexDiff::onRangeDialogAccepted);

    connect(this, &HexDiff::selectionChanged, this,
            [this](Selection newSelection) { actionCopy->setEnabled(!newSelection.empty); });

    updateMetrics();
    updateItemLength();

    startAddress = 0ULL;
    cursor.address = 0ULL;
    ctxA.data.reset(new MemoryData());
    ctxB.data.reset(new MemoryData());
    ctxA.file = DiffFile::A;
    ctxB.file = DiffFile::B;

    fetchData();
    updateCursorMeta();

    connect(&cursor.blinkTimer, &QTimer::timeout, this, &HexDiff::onCursorBlinked);
    cursor.setBlinkPeriod(1000);
    cursor.startBlinking();

    updateColors();

    warningTimer.setSingleShot(true);
    connect(&warningTimer, &QTimer::timeout, this, &HexDiff::hideWarningRect);
}

void HexDiff::setMonospaceFont(const QFont &font)
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

    updateViewport();
}

void HexDiff::setItemSize(int nbytes)
{
    static const QVector<int> values({ 1, 2, 4, 8 });

    if (!values.contains(nbytes)) {
        return;
    }

    itemByteLen = nbytes;
    if (itemByteLen > rowSizeBytes) {
        rowSizeBytes = itemByteLen;
    }

    actionsItemFormat.at(ItemFormatFloat)->setEnabled(nbytes >= 4);
    actionItemBigEndian->setEnabled(nbytes != 1);

    updateItemLength();
    if (cursorArea > 1 && cursor.address % itemByteLen) {
        moveCursor(-int(cursor.address % itemByteLen));
    }
    fetchData();
    updateCursorMeta();

    updateViewport();
}

void HexDiff::setItemFormat(ItemFormat format)
{

    itemFormat = format;

    bool sizeEnabled = true;
    if (format == ItemFormatFloat) {
        sizeEnabled = false;
    }
    actionsItemSize.at(0)->setEnabled(sizeEnabled);
    actionsItemSize.at(1)->setEnabled(sizeEnabled);

    updateItemLength();
    fetchData();
    updateCursorMeta();

    updateViewport();
}

void HexDiff::setItemGroupSize(int size)
{
    itemGroupSize = size;

    updateCounts();
    fetchData();
    updateCursorMeta();

    updateViewport();
}

void HexDiff::updateCounts()
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
        int lastGoodSize = itemGroupByteLen();
        for (int i = itemGroupByteLen(); i <= maxLineWidthBytes; i *= 2) {
            rowSizeBytes = i;
            itemColumns = rowSizeBytes / itemGroupByteLen();
            updateAreasPosition();
            if (horizontalScrollBar()->maximum() == 0) {
                lastGoodSize = rowSizeBytes;
            } else {
                break;
            }
        }
        rowSizeBytes = lastGoodSize;
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
            if (w > maxLineWidthPreset) {
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

void HexDiff::setFixedLineSize(int lineSize)
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

    updateViewport();
}

void HexDiff::setColumnMode(ColumnMode mode)
{
    columnMode = mode;

    updateCounts();
    fetchData();
    updateCursorMeta();

    updateViewport();
}

void HexDiff::selectRange(RVA start, RVA end)
{
    BasicDiffCursor endCursor(end);
    endCursor += 1;
    setCursorAddr(endCursor);
    selection.set(start, end);
    cursorEnabled = false;
    emit selectionChanged(getSelection());
}

void HexDiff::clearSelection()
{
    setCursorAddr(BasicDiffCursor(cursor.address), false);
    emit selectionChanged(getSelection());
}

HexDiff::Selection HexDiff::getSelection()
{
    return Selection { selection.isEmpty(), selection.start(), selection.end() };
}

void HexDiff::seek(uint64_t address)
{
    if (cursorArea > 1) {
        // when other widget causes seek to the middle of word
        // switch to ascii column which operates with byte positions
        auto viewOffset = startAddress % itemByteLen;
        auto addrOffset = address % itemByteLen;
        if ((addrOffset + itemByteLen - viewOffset) % itemByteLen) {
            setCursorOnAscii(true);
        }
    }
    setCursorAddr(BasicDiffCursor(address));
}

void HexDiff::refresh()
{
    fetchData();
    updateViewport();
}

void HexDiff::setItemEndianness(bool bigEndian)
{
    itemBigEndian = bigEndian;

    updateCursorMeta(); // Update cached item character

    updateViewport();
}

void HexDiff::updateColors()
{
    borderColor = Config()->getColor("gui.border");
    backgroundColor = Config()->getColor("gui.background");
    b0x00Color = Config()->getColor("b0x00");
    b0x7fColor = Config()->getColor("b0x7f");
    b0xffColor = Config()->getColor("b0xff");
    printableColor = Config()->getColor("ai.write");
    defColor = Config()->getColor("btext");
    addrColor = Config()->getColor("func_var_addr");
    diffColor = Config()->getColor("graph.diff.unmatch");
    warningColor = QColor("red");

    updateCursorMeta();
    updateViewport();
}

void HexDiff::paintEvent(QPaintEvent *event)
{
    QPainter painter(viewport());
    painter.setFont(monospaceFont);

    const int xOffset = horizontalScrollBar()->value();
    if (xOffset > 0) {
        painter.translate(QPoint(-xOffset, 0));
    }

    const QRect cursorRectA = cursor.screenPos.toAlignedRect();
    const QRect cursorRectB = cursor.screenPosB.toAlignedRect();
    if (event->rect() == cursorRectA || event->rect() == cursorRectB) {
        /*cursor blink*/
        drawCursor(painter);
        return;
    }

    painter.fillRect(event->rect().translated(xOffset, 0), backgroundColor);

    drawHeader(painter, ctxA);
    drawHeader(painter, ctxB);

    drawAddrArea(painter, ctxA);
    drawItemArea(painter, ctxA);
    drawAsciiArea(painter, ctxA);

    drawItemArea(painter, ctxB);
    drawAsciiArea(painter, ctxB);
    drawAddrArea(painter, ctxB);

    if (warningRectVisible) {
        painter.setPen(warningColor);
        painter.drawRect(warningRect);
    }

    if (!cursorEnabled) {
        return;
    }

    drawCursor(painter, true);
}

void HexDiff::updateWidth()
{
    int max = (showAscii ? ctxB.asciiArea.right() : ctxB.itemArea.right()) - viewport()->width();
    if (max < 0) {
        max = 0;
    } else {
        max += charWidth;
    }
    horizontalScrollBar()->setMaximum(max);
    horizontalScrollBar()->setSingleStep(charWidth);
}

bool HexDiff::isFixedWidth() const
{
    return itemFormat == ItemFormatHex || itemFormat == ItemFormatOct;
}

void HexDiff::resizeEvent(QResizeEvent *event)
{
    const int oldByteCount = bytesPerScreen();
    updateCounts();

    if (event->oldSize().height() == event->size().height() && oldByteCount == bytesPerScreen()) {
        return;
    }

    updateAreasHeight();
    fetchData(); // rowCount was changed
    updateCursorMeta();

    updateViewport();
}

DiffFile HexDiff::getFileFromPos(QPoint &pos)
{
    if (ctxA.asciiArea.contains(pos) || ctxA.itemArea.contains(pos)) {
        return DiffFile::A;
    }
    return DiffFile::B;
}

void HexDiff::mouseMoveEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();
    pos.rx() += horizontalScrollBar()->value();

    const bool cursorOnArea = ctxA.asciiArea.contains(pos) || ctxB.asciiArea.contains(pos)
            || ctxA.itemArea.contains(pos) || ctxB.itemArea.contains(pos);

    auto mouseAddr = mousePosToAddrA(pos).address;

    QString infoText;
    if (!updatingSelection && (cursorOnArea)) {
        QString metaData = getFileFromPos(pos) == DiffFile::A
                ? getFlagsAndComment(mouseAddr, ctxA)
                : getFlagsAndComment(getAddressB(mouseAddr), ctxB); // has to be redefined with ctx
        if (!metaData.isEmpty() && (ctxA.itemArea.contains(pos) || ctxB.itemArea.contains(pos))) {
            infoText = metaData.replace(",", ", ");
        }

        const auto marks = Core()->getMarksAt(mouseAddr);
        for (const auto &mark : marks) {
            if (mark.realname.isEmpty()) {
                continue;
            }
            if (!infoText.isEmpty()) {
                infoText += "<br>";
            }
            const QColor c = mark.color;
            infoText += QString("<span style='white-space:nowrap; color: rgba(%1, %2, %3, %4);'>● "
                                "</span> %5")
                                .arg(c.red())
                                .arg(c.green())
                                .arg(c.blue())
                                .arg(markAlphaF)
                                .arg(mark.realname.toHtmlEscaped());
        }
        if (!infoText.isEmpty()) {
            // forces tooltip to follow cursor movement
            QToolTip::showText(mapToGlobal(event->pos()), infoText + " ", this);

            QToolTip::showText(mapToGlobal(event->pos()), infoText, this);
        } else {
            QToolTip::hideText();
        }
    } else {
        QToolTip::hideText();
    }

    if (!updatingSelection) {
        if (cursorOnArea) {
            setCursor(Qt::IBeamCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }
        return;
    }

    auto &area = currentArea();
    if (pos.x() < area.left()) {
        pos.setX(area.left());
    } else if (pos.x() > area.right()) {
        pos.setX(area.right());
    }
    auto addr = currentAreaPosToAddrA(pos, true);
    setCursorAddr(addr, true);

    /* Stop blinking */
    cursorEnabled = false;

    updateViewport();
}

const QRectF &HexDiff::currentArea()
{
    switch (cursorArea) {
    case DiffArea::AsciiA:
        return ctxA.asciiArea;
    case DiffArea::AsciiB:
        return ctxB.asciiArea;
    case DiffArea::ItemA:
        return ctxA.itemArea;
    case DiffArea::ItemB:
        return ctxB.itemArea;
    }
    return ctxA.itemArea;
}

DiffArea HexDiff::posToDiffArea(QPoint &point) const
{
    if (ctxB.itemArea.contains(point)) {
        return DiffArea::ItemB;
    } else if (ctxA.asciiArea.contains(point)) {
        return DiffArea::AsciiA;
    } else if (ctxB.asciiArea.contains(point)) {
        return DiffArea::AsciiB;
    }
    return DiffArea::ItemA;
}

bool HexDiff::diffItemsAt(uint64_t addrA)
{
    quint8 a[8];
    quint8 b[8];
    ctxA.data->copy(a, addrA, static_cast<size_t>(itemByteLen));
    ctxB.data->copy(b, getAddressB(addrA), static_cast<size_t>(itemByteLen));
    return memcmp(a, b, itemByteLen);
}

bool HexDiff::diffByteArrays(QByteArray &a, QByteArray &b)
{
    return a == b;
}

void HexDiff::mousePressEvent(QMouseEvent *event)
{
    QPoint pos(event->pos());
    pos.rx() += horizontalScrollBar()->value();

    if (event->button() == Qt::LeftButton) {
        const DiffArea area = posToDiffArea(pos);
        const bool selectingData = area > DiffArea::AsciiB;
        const bool selecting = selectingData || area <= DiffArea::AsciiB;
        const bool holdingShift = event->modifiers() == Qt::ShiftModifier;

        if (selecting) {
            updatingSelection = true;
            setCursorOnArea(area);
            auto cursorPosition = currentAreaPosToAddrA(pos, true);
            setCursorAddr(cursorPosition, holdingShift);
            updateViewport();
        }
    }
}

void HexDiff::mouseDoubleClickEvent(QMouseEvent *event)
{
    QPoint pos(event->pos());
    pos.rx() += horizontalScrollBar()->value();
}

void HexDiff::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (selection.isEmpty()) {
            selection.init(BasicDiffCursor(cursor.address));
            cursorEnabled = true;
            updateViewport();
        }
        updatingSelection = false;
    }
}

void HexDiff::wheelEvent(QWheelEvent *event)
{

    // according to Qt doc 1 row per 5 degrees, angle measured in 1/8 of degree
    const int dy = event->angleDelta().y() / (8 * 5);
    scrollLines(dy);
    vScrollBar->showTransientScrollBar();
}

HexDiff::HexNavigationMode HexDiff::defaultNavigationMode()
{
    return HexNavigationMode::Words;
}

bool HexDiff::event(QEvent *event)
{
    // prefer treating keys like 's' 'g' '.' as typing input instead of global shortcuts
    if (event->type() == QEvent::ShortcutOverride) {
        auto keyEvent = static_cast<QKeyEvent *>(event);
        auto modifiers = keyEvent->modifiers();
        if ((modifiers == Qt::NoModifier || modifiers == Qt::ShiftModifier
             || modifiers == Qt::KeypadModifier)
            && keyEvent->key() < Qt::Key_Escape) {
            keyEvent->accept();
            return true;
        }
    }

    return QScrollArea::event(event);
}

void HexDiff::keyPressEvent(QKeyEvent *event)
{
    bool select = false;
    auto moveOrSelect = [event, &select](QKeySequence::StandardKey moveSeq,
                                         QKeySequence::StandardKey selectSeq) -> bool {
        if (event->matches(moveSeq)) {
            select = false;
            return true;
        } else if (event->matches(selectSeq)) {
            select = true;
            return true;
        }
        return false;
    };

    if (cursorArea < 2 || navigationMode == HexNavigationMode::Words
        || navigationMode == HexNavigationMode::AnyChar) {
        if (moveOrSelect(QKeySequence::MoveToNextPage, QKeySequence::SelectNextPage)) {
            moveCursor(bytesPerScreen(), select);
        } else if (moveOrSelect(QKeySequence::MoveToPreviousPage,
                                QKeySequence::SelectPreviousPage)) {
            moveCursor(-bytesPerScreen(), select);
        } else if (moveOrSelect(QKeySequence::MoveToStartOfLine, QKeySequence::SelectStartOfLine)) {
            const int linePos =
                    int((cursor.address % itemRowByteLen()) - (startAddress % itemRowByteLen()));
            moveCursor(-linePos, select);
        } else if (moveOrSelect(QKeySequence::MoveToEndOfLine, QKeySequence::SelectEndOfLine)) {
            const int linePos =
                    int((cursor.address % itemRowByteLen()) - (startAddress % itemRowByteLen()));
            moveCursor(itemRowByteLen() - linePos, select);
        }
    }

    if (navigationMode == HexNavigationMode::Words || cursorArea < 2) {
        if (moveOrSelect(QKeySequence::MoveToNextLine, QKeySequence::SelectNextLine)) {
            moveCursor(itemRowByteLen(), select, OverflowMove::Ignore);
        } else if (moveOrSelect(QKeySequence::MoveToPreviousLine,
                                QKeySequence::SelectPreviousLine)) {
            moveCursor(-itemRowByteLen(), select, OverflowMove::Ignore);
        } else if (moveOrSelect(QKeySequence::MoveToNextChar, QKeySequence::SelectNextChar)
                   || moveOrSelect(QKeySequence::MoveToNextWord, QKeySequence::SelectNextWord)) {
            moveCursor(cursorArea < 2 ? 1 : itemByteLen, select);
        } else if (moveOrSelect(QKeySequence::MoveToPreviousChar, QKeySequence::SelectPreviousChar)
                   || moveOrSelect(QKeySequence::MoveToPreviousWord,
                                   QKeySequence::SelectPreviousWord)) {
            moveCursor(cursorArea < 2 ? -1 : -itemByteLen, select);
        }
    } else if (navigationMode == HexNavigationMode::AnyChar && cursorArea < 1) {
        if (moveOrSelect(QKeySequence::MoveToNextChar, QKeySequence::SelectNextChar)) {
            if (select) {
                moveCursor(itemByteLen, select);
            } else {
                if (!selection.isEmpty()) {
                    clearSelection();
                }
            }
            updateViewport();
        } else if (event->matches(QKeySequence::SelectPreviousChar)) {
            moveCursor(-itemByteLen, true);
        } else if (moveOrSelect(QKeySequence::MoveToNextWord, QKeySequence::SelectNextWord)) {
            moveCursor(itemByteLen, select);
        } else if (event->matches(QKeySequence::MoveToPreviousWord)) {
            moveCursor(-itemByteLen, false);
        } else if (event->matches(QKeySequence::SelectPreviousWord)) {
            moveCursor(-itemByteLen, true);
        }
    }
}

void HexDiff::contextMenuEvent(QContextMenuEvent *event)
{
    const QPoint pt = event->pos();
    bool mouseOutsideSelection = false;
    if (event->reason() == QContextMenuEvent::Mouse) {
        auto mouseAddr = mousePosToAddrA(pt).address;
        if (ctxA.asciiArea.contains(pt)) {
            cursorArea = DiffArea::AsciiA;
        } else if (ctxB.asciiArea.contains(pt)) {
            cursorArea = DiffArea::AsciiB;
        } else if (ctxB.itemArea.contains(pt)) {
            cursorArea = DiffArea::ItemB;
        } else {
            cursorArea = DiffArea::ItemA;
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

    auto *menu = new QMenu(this);
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

void HexDiff::onCursorBlinked()
{
    if (!cursorEnabled) {
        return;
    }
    cursor.blink();
    const int xOffset = horizontalScrollBar()->value();
    viewport()->update(cursor.screenPos.toAlignedRect().translated(-xOffset, 0));
    viewport()->update(cursor.screenPosB.toAlignedRect().translated(-xOffset, 0));
}

void HexDiff::onHexPairsModeEnabled(bool enable)
{
    // Sync configuration
    Core()->setConfig("hex.pairs", enable);
    if (enable) {
        setItemGroupSize(2);
    } else {
        setItemGroupSize(1);
    }
}

void HexDiff::copy()
{ // needs to be changed double cores
    if (selection.isEmpty() || selection.size() > maxCopySize) {
        return;
    }

    auto x = cursorArea < 2
            ? Core()->getString(selection.start(), selection.size(), RZ_STRING_ENC_8BIT, true)
            : Core()->ioRead(selection.start(), (int)selection.size()).toHex();
    QApplication::clipboard()->setText(x);
}

void HexDiff::copyAddress()
{
    const uint64_t addr = getLocationAddress();
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(rzAddressString(addr));
}

void HexDiff::onRangeDialogAccepted()
{
    if (rangeDialog.empty()) {
        seek(rangeDialog.getStartAddress());
        return;
    }
    selectRange(rangeDialog.getStartAddress(), rangeDialog.getEndAddress());
}

void HexDiff::updateItemLength()
{
    itemPrefixLen = 0;
    itemPrefix.clear();

    switch (itemFormat) {
    case ItemFormatHex:
        itemCharLen = 2 * itemByteLen;
        if (itemByteLen > 1 && showExHex) {
            itemPrefixLen = hexPrefix.length();
            itemPrefix = hexPrefix;
        }
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
        if (itemByteLen < 4) {
            itemByteLen = 4;
        }
        // FIXME
        itemCharLen = 3 * itemByteLen;
        break;
    }

    itemCharLen += itemPrefixLen;

    updateCounts();
}

void HexDiff::drawHeader(QPainter &painter, DiffFileContext &ctx)
{
    if (!showHeader) {
        return;
    }

    int offset = 0;
    QRectF rect(ctx.itemArea.left(), 0, itemWidth(), lineHeight);

    painter.setPen(addrColor);

    for (int j = 0; j < itemColumns; ++j) {
        for (int k = 0; k < itemGroupSize; ++k, offset += itemByteLen) {
            painter.drawText(rect, Qt::AlignVCenter | Qt::AlignRight,
                             QString::number(offset, 16).toUpper());
            rect.translate(itemWidth(), 0);
        }
        rect.translate(columnSpacingWidth(), 0);
    }

    rect.moveLeft(ctx.asciiArea.left());
    rect.setWidth(charWidth);
    for (int j = 0; j < itemRowByteLen(); ++j) {
        painter.drawText(rect, Qt::AlignVCenter | Qt::AlignRight,
                         QString::number(j % 16, 16).toUpper());
        rect.translate(charWidth, 0);
    }
}

void HexDiff::drawCursor(QPainter &painter, bool shadow)
{
    if (shadow) {
        QPen pen(Qt::gray);
        pen.setStyle(Qt::DashLine);
        painter.setPen(pen);
        qreal shadowWidth = charWidth;
        if (cursorArea < 2) {
            shadowWidth = itemWidth();
        }
        shadowCursor.screenPos.setWidth(shadowWidth);
        shadowCursor.screenPosB.setWidth(shadowWidth);
        painter.drawRect(shadowCursor.screenPos);
        painter.drawRect(shadowCursor.screenPosB);
        painter.setPen(Qt::SolidLine);
    }

    painter.setPen(cursor.cachedColor);
    QRectF charRect(cursor.screenPos);
    charRect.setWidth(charWidth);
    painter.fillRect(charRect, backgroundColor);
    painter.drawText(charRect, Qt::AlignVCenter, cursor.cachedChar);
    painter.setPen(cursor.cachedColorB);
    QRectF charRectB(cursor.screenPosB);
    charRectB.setWidth(charWidth);
    painter.fillRect(charRectB, backgroundColor);
    painter.drawText(charRectB, Qt::AlignVCenter, cursor.cachedCharB);
    if (cursor.isVisible) {
        painter.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
        painter.fillRect(cursor.screenPos, QColor(0xff, 0xff, 0xff));
        painter.fillRect(cursor.screenPosB, QColor(0xff, 0xff, 0xff));
    }
}

void HexDiff::drawAddrArea(QPainter &painter, DiffFileContext &ctx)
{

    uint64_t offset = ctx.file == DiffFile::A ? startAddress : getStartAddressB();

    QString addrString;
    const QSizeF areaSize((addrCharLen + (showExAddr ? 2 : 0)) * charWidth, lineHeight);
    QRectF strRect(ctx.addrArea.topLeft(), areaSize);

    painter.setPen(addrColor);
    for (int line = 0; line < visibleLines && offset <= ctx.data->maxIndex();
         ++line, strRect.translate(0, lineHeight), offset += itemRowByteLen()) {
        addrString = QString("%1").arg(offset, addrCharLen, 16, QLatin1Char('0'));
        if (showExAddr) {
            addrString.prepend(hexPrefix);
        }
        painter.drawText(strRect, Qt::AlignVCenter, addrString);
    }

    painter.setPen(borderColor);

    const qreal vLineOffset = ctx.itemArea.left() - charWidth;
    painter.drawLine(QLineF(vLineOffset, 0, vLineOffset, viewport()->height()));
}

uint64_t HexDiff::ctxAddr(uint64_t addrA, DiffFileContext &ctx)
{
    if (ctxA.file == DiffFile::A) {
        return getAddressB(addrA);
    }
    return addrA;
}

HexDiffSelection HexDiff::ctxSelection(DiffFileContext &ctx)
{
    if (ctx.file == DiffFile::A) {
        return selection;
    }
    HexDiffSelection selectionB;
    if (!selection.isEmpty()) {
        selectionB.set(getAddressB(selection.start()), getAddressB(selection.end()));
    }
    return selectionB;
}

void HexDiff::drawItemArea(QPainter &painter, DiffFileContext &ctx)
{
    const uint64_t addr = ctx.file == DiffFile::A ? startAddress : getStartAddressB();

    QRectF itemRect(ctx.itemArea.topLeft(), QSizeF(itemWidth(), lineHeight));
    QColor itemColor;
    QString itemString;

    fillSelectionBackground(painter, ctx);

    const HexDiffSelection &ctxSel = ctxSelection(ctx);

    uint64_t itemAddr = addr;
    auto &itemCursor = cursorArea < 2 ? shadowCursor : cursor;
    for (int line = 0; line < visibleLines; ++line) {
        itemRect.moveLeft(ctx.itemArea.left());
        for (int j = 0; j < itemColumns; ++j) {
            for (int k = 0; k < itemGroupSize && itemAddr <= ctx.data->maxIndex();
                 ++k, itemAddr += itemByteLen) {

                itemString = renderItem(itemAddr - addr, ctx, &itemColor);

                if (!getFlagsAndComment(itemAddr, ctx).isEmpty()) {
                    QColor markerColor(borderColor);
                    markerColor.setAlphaF(0.5);
                    painter.setPen(markerColor);
                    for (const auto &shape : rangePolygons(itemAddr, itemAddr, false, ctx)) {
                        painter.drawPolyline(shape);
                    }
                }
                if (ctxSel.contains(itemAddr) && cursorArea > 1) {
                    itemColor = palette().highlightedText().color();
                }

                painter.setPen(itemColor);
                painter.drawText(itemRect, Qt::AlignVCenter, itemString);
                itemRect.translate(itemWidth(), 0);
                if (ctx.file == DiffFile::A && itemAddr == cursor.address) {
                    itemCursor.cachedChar = itemString.at(0);
                    itemCursor.cachedColor = itemColor;
                }
                if (ctx.file == DiffFile::B && itemAddr == getAddressB(cursor.address)) {
                    itemCursor.cachedCharB = itemString.at(0);
                    itemCursor.cachedColorB = itemColor;
                }
            }
            itemRect.translate(columnSpacingWidth(), 0);
        }
        itemRect.translate(0, lineHeight);
    }

    painter.setPen(borderColor);

    const qreal vLineOffset = ctx.asciiArea.left() - charWidth;
    painter.drawLine(QLineF(vLineOffset, 0, vLineOffset, viewport()->height()));
}

void HexDiff::drawAsciiArea(QPainter &painter, DiffFileContext &ctx)
{
    const uint64_t addr = ctx.file == DiffFile::A ? startAddress : getStartAddressB();
    QRectF charRect(ctx.asciiArea.topLeft(), QSizeF(charWidth, lineHeight));

    fillSelectionBackground(painter, ctx, true);
    painter.setBrush(Qt::NoBrush);

    uint64_t address = addr;
    QChar ascii;
    QColor color;
    auto &itemCursor = cursorArea < 2 ? cursor : shadowCursor;
    for (int line = 0; line < visibleLines; ++line, charRect.translate(0, lineHeight)) {
        charRect.moveLeft(ctx.asciiArea.left());
        for (int j = 0; j < itemRowByteLen() && address <= ctx.data->maxIndex(); ++j, ++address) {
            ascii = renderAscii(address - addr, ctx, &color);
            if (selection.contains(address) && cursorArea < 2) {
                color = palette().highlightedText().color();
            }
            painter.setPen(color);
            /* Dots look ugly. Use fillRect() instead of drawText(). */
            if (ascii == '.') {
                const qreal a = cursor.screenPos.width();
                QPointF p = charRect.bottomLeft();
                p.rx() += (charWidth - a) / 2 + 1;
                p.ry() += -2 * a;
                painter.fillRect(QRectF(p, QSizeF(a, a)), color);
            } else {
                painter.drawText(charRect, Qt::AlignVCenter, ascii);
            }
            charRect.translate(charWidth, 0);
            if (ctx.file == DiffFile::A && cursor.address == address) {
                itemCursor.cachedChar = ascii;
                itemCursor.cachedColor = color;
            }
            if (ctx.file == DiffFile::B && getAddressB(cursor.address) == address) {
                itemCursor.cachedCharB = ascii;
                itemCursor.cachedColorB = color;
            }
        }
    }
}

void HexDiff::fillSelectionBackground(QPainter &painter, DiffFileContext &ctx, bool ascii)
{
    if (selection.isEmpty()) {
        return;
    }
    const auto parts =
            rangePolygons(ctxSelection(ctx).start(), ctxSelection(ctx).end(), ascii, ctx);
    for (const auto &shape : parts) {
        const QColor highlightColor = palette().color(QPalette::Highlight);
        if (ascii == cursorArea < 2) {
            painter.setBrush(highlightColor);
            painter.drawPolygon(shape);
        } else {
            painter.setPen(highlightColor);
            painter.drawPolyline(shape);
        }
    }
}

QVector<QPolygonF> HexDiff::rangePolygons(RVA start, RVA last, bool ascii, DiffFileContext &ctx)
{
    const uint64_t addr = ctx.file == DiffFile::A ? startAddress : getStartAddressB();

    if (last < addr || start > lastVisibleAddr(ctx.file)) {
        return {};
    }

    QRectF rect;
    const QRectF area = QRectF(ascii ? ctx.asciiArea : ctx.itemArea);

    /* Convert absolute values to relative */
    const int startOffset = std::max(uint64_t(start), addr) - addr;
    const int endOffset = std::min(uint64_t(last), lastVisibleAddr(ctx.file)) - addr;

    QVector<QPolygonF> parts;

    auto getRectangle = [&](int offset) {
        return QRectF(ascii ? asciiRectangle(offset, ctx) : itemRectangle(offset, ctx));
    };

    auto startRect = getRectangle(startOffset);
    auto endRect = getRectangle(endOffset);
    bool startJagged = false;
    bool endJagged = false;
    if (!ascii) {
        if (const int startFraction = startOffset % itemByteLen) {
            startRect.setLeft(startRect.left() + startFraction * startRect.width() / itemByteLen);
            startJagged = true;
        }
        if (const int endFraction = itemByteLen - 1 - (endOffset % itemByteLen)) {
            endRect.setRight(endRect.right() - endFraction * endRect.width() / itemByteLen);
            endJagged = true;
        }
    }
    if (endOffset - startOffset + 1 <= rowSizeBytes) {
        if (startOffset / rowSizeBytes == endOffset / rowSizeBytes) { // single row
            rect = startRect;
            rect.setRight(endRect.right());
            parts.push_back(QPolygonF(rect));
        } else {
            // two separate rectangles
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
    if (!ascii && (startJagged || endJagged) && parts.length() >= 1) {

        QPolygonF top;
        top.reserve(3);
        top << QPointF(0, 0) << QPointF(charWidth, lineHeight / 3) << QPointF(0, lineHeight / 2);
        QPolygonF bottom;
        bottom.reserve(3);
        bottom << QPointF(0, lineHeight / 2) << QPointF(-charWidth, 2 * lineHeight / 3)
               << QPointF(0, lineHeight);

        // small adjustment to make sure that edges don't overlap with rect edges, QPolygonF doesn't
        // handle it properly
        const QPointF adjustment(charWidth / 16, 0);
        top.translate(-adjustment);
        bottom.translate(adjustment);

        if (startJagged) {
            auto movedTop = top.translated(startRect.topLeft());
            auto movedBottom = bottom.translated(startRect.topLeft());
            parts[0] = parts[0].subtracted(movedTop).united(movedBottom);
        }
        if (endJagged) {
            auto movedTop = top.translated(endRect.topRight());
            auto movedBottom = bottom.translated(endRect.topRight());
            parts.last() = parts.last().subtracted(movedBottom).united(movedTop);
        }
    }
    return parts;
}

void HexDiff::updateMetrics()
{
    const QFontMetricsF fontMetrics(this->monospaceFont);
    lineHeight = fontMetrics.height();
#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
    charWidth = fontMetrics.width('A');
#else
    charWidth = fontMetrics.horizontalAdvance('A');
#endif

    updateCounts();
    updateAreasHeight();

    const qreal cursorWidth = std::max(charWidth / 3, 1.);
    cursor.screenPos.setHeight(lineHeight);
    shadowCursor.screenPos.setHeight(lineHeight);

    cursor.screenPos.setWidth(cursorWidth);

    cursor.screenPosB.setHeight(lineHeight);
    shadowCursor.screenPosB.setHeight(lineHeight);

    cursor.screenPosB.setWidth(cursorWidth);
    if (cursorArea < 2) {
        cursor.screenPos.moveTopLeft(ctxA.asciiArea.topLeft());

        shadowCursor.screenPos.setWidth(itemWidth());
        shadowCursor.screenPos.moveTopLeft(ctxA.itemArea.topLeft());

        cursor.screenPosB.moveTopLeft(ctxB.asciiArea.topLeft());

        shadowCursor.screenPosB.setWidth(itemWidth());
        shadowCursor.screenPosB.moveTopLeft(ctxB.itemArea.topLeft());
    } else {
        cursor.screenPos.moveTopLeft(ctxA.itemArea.topLeft());
        shadowCursor.screenPos.setWidth(charWidth);
        shadowCursor.screenPos.moveTopLeft(ctxA.asciiArea.topLeft());

        cursor.screenPosB.moveTopLeft(ctxB.itemArea.topLeft());
        shadowCursor.screenPosB.setWidth(charWidth);
        shadowCursor.screenPosB.moveTopLeft(ctxB.asciiArea.topLeft());
    }
}

void HexDiff::updateAreasPosition()
{
    const qreal spacingWidth = areaSpacingWidth();

    const qreal yOffset = showHeader ? lineHeight : 0;

    ctxA.addrArea.setTopLeft(QPointF(0, yOffset));
    ctxA.addrArea.setWidth((addrCharLen + (showExAddr ? 2 : 0)) * charWidth);

    ctxA.itemArea.setTopLeft(QPointF(ctxA.addrArea.right() + spacingWidth, yOffset));
    ctxA.itemArea.setWidth(itemRowWidth());

    ctxA.asciiArea.setTopLeft(QPointF(ctxA.itemArea.right() + spacingWidth, yOffset));
    ctxA.asciiArea.setWidth(asciiRowWidth());

    ctxB.addrArea.setTopLeft(QPointF(ctxA.asciiArea.right() + spacingWidth, yOffset));
    ctxB.addrArea.setWidth((addrCharLen + (showExAddr ? 2 : 0)) * charWidth);

    ctxB.itemArea.setTopLeft(QPointF(ctxB.addrArea.right() + spacingWidth, yOffset));
    ctxB.itemArea.setWidth(itemRowWidth());

    ctxB.asciiArea.setTopLeft(QPointF(ctxB.itemArea.right() + spacingWidth, yOffset));
    ctxB.asciiArea.setWidth(asciiRowWidth());

    updateWidth();
}

void HexDiff::updateAreasHeight()
{
    visibleLines = static_cast<int>((viewport()->height() - ctxA.itemArea.top()) / lineHeight);

    const qreal height = visibleLines * lineHeight;
    ctxA.addrArea.setHeight(height);
    ctxA.itemArea.setHeight(height);
    ctxA.asciiArea.setHeight(height);
    ctxB.addrArea.setHeight(height);
    ctxB.itemArea.setHeight(height);
    ctxB.asciiArea.setHeight(height);
}

bool HexDiff::moveCursor(int offset, bool select, OverflowMove overflowMove)
{
    const uint64_t maxIndex = qMin(ctxA.data->maxIndex(), ctxB.data->maxIndex());
    BasicDiffCursor addr(cursor.address);
    if (overflowMove == OverflowMove::Ignore) {
        if (addr.moveChecked(offset)) {
            if (addr.address > maxIndex) {
                addr.address = maxIndex;
                addr.pastEnd = true;
            }
            setCursorAddr(addr, select);
            return true;
        }
        return false;
    } else {
        addr += offset;
        if (addr.address > maxIndex) {
            addr.address = maxIndex;
        }
        setCursorAddr(addr, select);
        return true;
    }
}

void HexDiff::setCursorAddr(BasicDiffCursor addr, bool select)
{
    if (!select) {
        const bool clearingSelection = !selection.isEmpty();
        selection.init(addr);
        if (clearingSelection) {
            emit selectionChanged(getSelection());
        }
    }
    emit positionChanged(addr.address);

    cursor.address = addr.address;
    if (cursorArea > 1) {
        cursor.address -= cursor.address % itemByteLen;
    }

    /* Pause cursor repainting */
    cursorEnabled = false;

    if (select) {
        selection.update(addr);
        emit selectionChanged(getSelection());
    }

    uint64_t addressValue = cursor.address;
    /* Update data cache if necessary */
    if (!(addressValue >= startAddress && addressValue <= lastVisibleAddr(DiffFile::A))) {
        /* Align start address */
        addressValue -= (addressValue % itemRowByteLen());

        /* FIXME: handling Page Up/Down */
        const uint64_t rowAfterVisibleAddress = startAddress + bytesPerScreen();
        if (addressValue == rowAfterVisibleAddress && addressValue > startAddress) {
            // when pressing down add only one new row
            startAddress += itemRowByteLen();
        } else {
            startAddress = addressValue;
        }

        fetchData();

        if (startAddress > (ctxA.data->maxIndex() - bytesPerScreen()) + 1) {
            startAddress = (ctxA.data->maxIndex() - bytesPerScreen()) + 1;
        }
    }

    updateCursorMeta();

    /* Draw cursor */
    cursor.isVisible = !select;
    updateViewport();

    /* Resume cursor repainting */
    cursorEnabled = selection.isEmpty();
}

void HexDiff::updateCursorMeta()
{
    QPointF point;
    QPointF pointAscii;

    const int offset = cursor.address - startAddress;
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

    QPointF pointB = point;
    QPointF pointAsciiB = pointAscii;

    point += ctxA.itemArea.topLeft();
    pointAscii += ctxA.asciiArea.topLeft();
    pointB += ctxB.itemArea.topLeft();
    pointAsciiB += ctxB.asciiArea.topLeft();

    cursor.screenPos.moveTopLeft(cursorArea < 2 ? pointAscii : point);
    shadowCursor.screenPos.moveTopLeft(cursorArea < 2 ? point : pointAscii);
    cursor.screenPosB.moveTopLeft(cursorArea < 2 ? pointAsciiB : pointB);
    shadowCursor.screenPosB.moveTopLeft(cursorArea < 2 ? pointB : pointAsciiB);
}

void HexDiff::setCursorOnAscii(bool ascii)
{
    if (cursorArea % 2) {
        if (ascii) {
            cursorArea = DiffArea::AsciiB;
        } else {
            cursorArea = DiffArea::ItemB;
        }
    } else {
        if (ascii) {
            cursorArea = DiffArea::AsciiB;
        } else {
            cursorArea = DiffArea::ItemB;
        }
    }
}

void HexDiff::setCursorOnArea(DiffArea area)
{
    cursorArea = area;
}

QColor HexDiff::itemColor(uint8_t byte)
{
    QColor color(defColor);

    if (byte == 0x00) {
        color = b0x00Color;
    } else if (byte == 0x7f) {
        color = b0x7fColor;
    } else if (byte == 0xff) {
        color = b0xffColor;
    } else if (IS_PRINTABLE(byte)) {
        color = printableColor;
    }

    return color;
}

template<class T>
static T fromBigEndian(const void *src)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
    return qFromBigEndian<T>(src);
#else
    T result;
    memcpy(&result, src, sizeof(T));
    return qFromBigEndian<T>(result);
#endif
}

template<class T>
static T fromLittleEndian(const void *src)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
    return qFromLittleEndian<T>(src);
#else
    T result;
    memcpy(&result, src, sizeof(T));
    return qFromLittleEndian<T>(result);
#endif
}

QVariant HexDiff::readItem(int offset, DiffFileContext &ctx, QColor *color)
{
    const uint64_t addr = ctx.file == DiffFile::A ? startAddress : getStartAddressB();
    quint8 byte;
    quint16 word;
    quint32 dword;
    quint64 qword;
    float float32;
    double float64;

    quint8 bytes[sizeof(uint64_t)];
    ctx.data->copy(bytes, addr + offset, static_cast<size_t>(itemByteLen));
    const bool signedItem = itemFormat == ItemFormatSignedDec;

    if (color) {
        *color = defColor;
    }

    switch (itemByteLen) {
    case 1:
        byte = bytes[0];
        if (color) {
            *color = itemColor(byte);
        }
        if (!signedItem) {
            return QVariant(static_cast<quint64>(byte));
        }
        return QVariant(static_cast<qint64>(static_cast<qint8>(byte)));
    case 2:
        if (itemBigEndian) {
            word = fromBigEndian<quint16>(bytes);
        } else {
            word = fromLittleEndian<quint16>(bytes);
        }

        if (!signedItem) {
            return QVariant(static_cast<quint64>(word));
        }
        return QVariant(static_cast<qint64>(static_cast<qint16>(word)));
    case 4:
        if (itemBigEndian) {
            dword = fromBigEndian<quint32>(bytes);
        } else {
            dword = fromLittleEndian<quint32>(bytes);
        }

        if (itemFormat == ItemFormatFloat) {
            memcpy(&float32, &dword, sizeof(float32));
            return QVariant(float32);
        }
        if (!signedItem) {
            return QVariant(static_cast<quint64>(dword));
        }
        return QVariant(static_cast<qint64>(static_cast<qint32>(dword)));
    case 8:
        if (itemBigEndian) {
            qword = fromBigEndian<quint64>(bytes);
        } else {
            qword = fromLittleEndian<quint64>(bytes);
        }
        if (itemFormat == ItemFormatFloat) {
            memcpy(&float64, &qword, sizeof(float64));
            return QVariant(float64);
        }
        if (!signedItem) {
            return QVariant(qword);
        }
        return QVariant(static_cast<qint64>(qword));
    }

    return QVariant();
}

QString HexDiff::renderItem(int offset, DiffFileContext &ctx, QColor *color)
{
    QString item;
    const QVariant itemVal = readItem(offset, ctx, color);
    const int itemLen = itemCharLen - itemPrefixLen; /* Reserve space for prefix */

    // FIXME: handle broken itemVal ( QVariant() )
    switch (itemFormat) {
    case ItemFormatHex:
        item = QString("%1").arg(itemVal.toULongLong(), itemLen, 16, QLatin1Char('0'));
        if (itemByteLen > 1 && showExHex) {
            item.prepend(hexPrefix);
        }
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
        item = QString("%1").arg(itemVal.toDouble(), itemLen, 'g', itemByteLen == 4 ? 6 : 15);
        break;
    }

    return item;
}

QChar HexDiff::renderAscii(int offset, DiffFileContext &ctx, QColor *color)
{
    const uint64_t addr = ctx.file == DiffFile::A ? startAddress : getStartAddressB();
    uchar byte;
    ctx.data->copy(&byte, addr + offset, sizeof(byte));
    if (color) {
        *color = itemColor(byte);
    }
    if (!IS_PRINTABLE(byte)) {
        byte = '.';
    }
    return QChar(byte);
}

/**
 * @brief Gets the available flags and comment at a specific address.
 * @param address Address of Item to be checked.
 * @return String containing the flags and comment available at the address.
 */
QString HexDiff::getFlagsAndComment(uint64_t address, DiffFileContext &ctx)
{ // Needs to be redefined using BinDiffClass so that it can be used for both cores
    const QString flagNames = Core()->listFlagsAsStringAt(address);
    QString metaData = flagNames.isEmpty() ? "" : "Flags: " + flagNames.trimmed();

    const QString comment = Core()->getCommentAt(address);
    if (!comment.isEmpty()) {
        if (!metaData.isEmpty()) {
            metaData.append("\n");
        }
        metaData.append("Comment: " + comment.trimmed());
    }

    return metaData;
}

template<class T, class BigValue>
static bool checkRange(BigValue v)
{
    return v >= std::numeric_limits<T>::min() && v <= std::numeric_limits<T>::max();
}

template<class T, class BigInteger>
static bool checkAndWrite(BigInteger value, uint8_t *buf, bool littleEndian)
{
    if (!checkRange<T>(value)) {
        return false;
    }
    if (littleEndian) {
        qToLittleEndian((T)value, buf);
    } else {
        qToBigEndian((T)value, buf);
    }
    return true;
}

template<class UType, class SType>
static bool checkAndWriteWithSign(const QVariant &value, uint8_t *buf, bool isSigned,
                                  bool littleEndian)
{
    if (isSigned) {
        return checkAndWrite<SType>(value.toLongLong(), buf, littleEndian);
    } else {
        return checkAndWrite<UType>(value.toULongLong(), buf, littleEndian);
    }
}

bool HexDiff::parseWord(const QString &word, uint8_t *buf, size_t bufferSize) const
{
    bool parseOk = false;
    if (bufferSize < size_t(itemByteLen)) {
        return false;
    }
    if (itemFormat == ItemFormatFloat) {
        if (itemByteLen == 4) {
            const float value = word.toFloat(&parseOk);
            if (!parseOk) {
                return false;
            }
            if (itemBigEndian) {
                rz_write_be_float(buf, value);
            } else {
                rz_write_le_float(buf, value);
            }
            return true;
        } else if (itemByteLen == 8) {
            const double value = word.toDouble(&parseOk);
            if (!parseOk) {
                return false;
            }
            if (itemBigEndian) {
                rz_write_be_double(buf, value);
            } else {
                rz_write_le_double(buf, value);
            }
            return true;
        }
        return false;
    } else {
        QVariant value;
        bool isSigned = false;
        switch (itemFormat) {
        case ItemFormatHex:
            value = word.toULongLong(&parseOk, 16);
            break;
        case ItemFormatOct:
            value = word.toULongLong(&parseOk, 8);
            break;
        case ItemFormatDec:
            value = word.toULongLong(&parseOk, 10);
            break;
        case ItemFormatSignedDec:
            isSigned = true;
            value = word.toLongLong(&parseOk, 10);
            break;
        default:
            break;
        }
        if (!parseOk) {
            return false;
        }

        switch (itemByteLen) {
        case 1:
            return checkAndWriteWithSign<uint8_t, int8_t>(value, buf, isSigned, !itemBigEndian);
        case 2:
            return checkAndWriteWithSign<uint16_t, int16_t>(value, buf, isSigned, !itemBigEndian);
        case 4:
            return checkAndWriteWithSign<quint32, qint32>(value, buf, isSigned, !itemBigEndian);
        case 8:
            return checkAndWriteWithSign<quint64, qint64>(value, buf, isSigned, !itemBigEndian);
        }
    }
    return false;
}

void HexDiff::fetchData()
{
    ctxA.data->fetch(startAddress, bytesPerScreen());
    ctxB.data->fetch(getStartAddressB(), bytesPerScreen());
}

const QRectF &HexDiff::screenPosToArea(const QPoint &point) const
{
    if (ctxA.itemArea.contains(point)) {
        return ctxA.itemArea;
    } else if (ctxA.asciiArea.contains(point)) {
        return ctxA.asciiArea;
    } else if (ctxB.itemArea.contains(point)) {
        return ctxB.itemArea;
    } else if (ctxB.asciiArea.contains(point)) {
        return ctxB.asciiArea;
    }
    return ctxA.asciiArea;
}

BasicDiffCursor HexDiff::screenPosToAddrA(const QPoint &point, bool middle, int *wordOffset) const
{
    QPointF pt = point - screenPosToArea(point).topLeft();

    int relativeAddress = 0;
    const int line = static_cast<int>(pt.y() / lineHeight);
    relativeAddress += line * itemRowByteLen();
    const int column = static_cast<int>(pt.x() / columnExWidth());
    relativeAddress += column * itemGroupByteLen();
    pt.rx() -= column * columnExWidth();
    auto roundingOffset = middle ? itemWidth() / 2 : 0;
    int posInGroup = static_cast<int>((pt.x() + roundingOffset) / itemWidth());
    if (!middle) {
        posInGroup = std::min(posInGroup, itemGroupSize - 1);
    }
    relativeAddress += posInGroup * itemByteLen;
    pt.rx() -= posInGroup * itemWidth();
    BasicDiffCursor result(startAddress);
    result += relativeAddress;

    if (!middle && wordOffset != nullptr) {
        int charPos = static_cast<int>((pt.x() / charWidth) + 0.5);
        charPos -= itemPrefixLen;
        charPos = std::max(0, charPos);
        *wordOffset = charPos;
    }
    return result;
}

BasicDiffCursor HexDiff::asciiPosToAddrA(const QPoint &point, bool middle) const
{
    const QPointF pt = point - screenPosToArea(point).topLeft();

    int relativeAddress = 0;
    relativeAddress += static_cast<int>(pt.y() / lineHeight) * itemRowByteLen();
    auto roundingOffset = middle ? (charWidth / 2) : 0;
    relativeAddress += static_cast<int>((pt.x() + (roundingOffset)) / charWidth);
    BasicDiffCursor result(startAddress);
    result += relativeAddress;

    return result;
}

BasicDiffCursor HexDiff::currentAreaPosToAddrA(const QPoint &point, bool middle) const
{
    return cursorArea < 2 ? asciiPosToAddrA(point, middle) : screenPosToAddrA(point, middle);
}

BasicDiffCursor HexDiff::mousePosToAddrA(const QPoint &point, bool middle) const
{
    return (ctxA.asciiArea.contains(point) || ctxB.asciiArea.contains(point))
            ? asciiPosToAddrA(point, middle)
            : screenPosToAddrA(point, middle);
}

QRectF HexDiff::itemRectangle(int offset, DiffFileContext &ctx)
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

    x += ctx.itemArea.x();
    y += ctx.itemArea.y();

    return QRectF(x, y, width, lineHeight);
}

QRectF HexDiff::asciiRectangle(int offset, DiffFileContext &ctx)
{
    QPointF p;

    p.ry() = (offset / itemRowByteLen()) * lineHeight;
    offset %= itemRowByteLen();

    p.rx() = offset * charWidth;

    p += ctx.asciiArea.topLeft();

    return QRectF(p, QSizeF(charWidth, lineHeight));
}

RVA HexDiff::getLocationAddress()
{
    return !selection.isEmpty() ? selection.start() : cursor.address;
}

void HexDiff::hideWarningRect()
{
    warningRectVisible = false;
    updateViewport();
}

void HexDiff::showWarningRect(QRectF rect)
{
    warningRect = rect;
    warningRectVisible = true;
    warningTimer.start(warningTimeMs);
    updateViewport();
}

void HexDiff::updateViewport()
{
    vScrollBar->setPosition(startAddress);
    viewport()->update();
}

void HexDiff::scrollLines(int lines, bool clampToScrollBarRange)
{
    const uint64_t maxIndex = qMin(ctxA.data->maxIndex(), ctxB.data->maxIndex());
    const int64_t delta = -lines * itemRowByteLen();

    if (lines == 0) {
        return;
    }

    if (delta < 0 && startAddress < static_cast<uint64_t>(-delta)) {
        startAddress = 0;
    } else if (delta > 0 && maxIndex < static_cast<uint64_t>(bytesPerScreen())) {
        startAddress = 0;
    } else if ((maxIndex - startAddress) <= static_cast<uint64_t>(bytesPerScreen() + delta - 1)) {
        startAddress = (maxIndex - bytesPerScreen()) + 1;
    } else {
        startAddress += delta;
    }

    if (clampToScrollBarRange) {
        startAddress = vScrollBar->clampAddressToRange(startAddress);
    }
    fetchData();

    updateCursorStatus();
    updateViewport();
}

void HexDiff::setStartAddress(RVA address)
{
    RVA aligned = address - (address % itemByteLen);

    const uint64_t maxIdx = qMin(ctxA.data->maxIndex(), ctxB.data->maxIndex());
    const uint64_t screenBytes = bytesPerScreen();
    if (maxIdx > screenBytes) {
        RVA maxStart = (maxIdx - screenBytes + 1);
        maxStart -= (maxStart % itemByteLen);
        aligned = std::min(aligned, maxStart);
    } else {
        aligned = 0;
    }

    // if (aligned == startAddress) {
    //     return;
    // }
    startAddress = aligned;
    fetchData();

    updateCursorStatus();
    updateViewport();
}

void HexDiff::shiftStartAddress(int shift)
{
    if (shift < 0) {
        setStartAddress(startAddress - qAbs(shift));
        return;
    }
    setStartAddress(startAddress + shift);
}

void HexDiff::transpose(int transA, int transB)
{
    relTranspose += transB * itemByteLen - transA * itemByteLen;
    shiftStartAddress(transA * itemByteLen);
    clearSelection();
    moveCursor(-transB);
}

void HexDiff::updateCursorStatus()
{
    if (cursor.address >= startAddress && cursor.address <= lastVisibleAddr(DiffFile::A)) {
        /* Don't enable cursor blinking if selection isn't empty */
        cursorEnabled = selection.isEmpty();
        updateCursorMeta();
    } else {
        cursorEnabled = false;
    }
}
