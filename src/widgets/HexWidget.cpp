#include "HexWidget.h"
#include "Cutter.h"
#include "Configuration.h"
#include "dialogs/MarkDialog.h"
#include "dialogs/WriteCommandsDialogs.h"
#include "dialogs/CommentsDialog.h"
#include "dialogs/FlagDialog.h"
#include "shortcuts/ShortcutManager.h"

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
#include <QInputDialog>
#include <QPushButton>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QToolTip>
#include <QActionGroup>

static constexpr uint64_t MAX_COPY_SIZE = 128 * 1024 * 1024;
static constexpr int MAX_LINE_WIDTH_PRESET = 32;
static constexpr int MAX_LINE_WIDTH_BYTES = 128 * 1024;
static constexpr int WARNING_TIME_MS = 500;

HexWidget::HexWidget(QWidget *parent)
    : QScrollArea(parent),
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
      showExAddr(true),
      ioModesController(parent),
      warningTimer(this)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::FocusPolicy::StrongFocus);
    connect(horizontalScrollBar(), &QScrollBar::valueChanged, this,
            [this]() { viewport()->update(); });

    connect(Config(), &Configuration::colorsUpdated, this, &HexWidget::updateColors);
    connect(Config(), &Configuration::fontsUpdated, this,
            [this]() { setMonospaceFont(Config()->getFont()); });

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
        connect(action, &QAction::triggered, this,
                [=]() { setItemFormat(static_cast<ItemFormat>(i)); });
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
    connect(actionRowSizePowerOf2, &QAction::triggered, this,
            [=]() { setColumnMode(ColumnMode::PowerOf2); });
    rowSizeMenu->addAction(actionRowSizePowerOf2);

    actionItemBigEndian = new QAction(tr("Big Endian"), this);
    actionItemBigEndian->setCheckable(true);
    actionItemBigEndian->setEnabled(false);
    connect(actionItemBigEndian, &QAction::triggered, this, &HexWidget::setItemEndianness);

    actionHexPairs = new QAction(tr("Bytes as pairs"), this);
    actionHexPairs->setCheckable(true);
    connect(actionHexPairs, &QAction::triggered, this, &HexWidget::onHexPairsModeEnabled);

    actionCopy = Shortcuts()->makeAction("Hex.copy", this);
    addAction(actionCopy);
    actionCopy->setShortcutContext(Qt::ShortcutContext::WidgetWithChildrenShortcut);
    connect(actionCopy, &QAction::triggered, this, &HexWidget::copy);

    actionCopyAddress = Shortcuts()->makeAction("General.copyAddress", this);
    actionCopyAddress->setShortcutContext(Qt::ShortcutContext::WidgetWithChildrenShortcut);
    connect(actionCopyAddress, &QAction::triggered, this, &HexWidget::copyAddress);
    addAction(actionCopyAddress);

    // Add comment option
    actionComment = Shortcuts()->makeAction("General.addComment", this);
    actionComment->setShortcutContext(Qt::ShortcutContext::WidgetWithChildrenShortcut);
    connect(actionComment, &QAction::triggered, this, &HexWidget::onActionAddCommentTriggered);
    addAction(actionComment);

    // Add flag option
    actionAddFlag = Shortcuts()->makeAction("Hex.addFlag", this);
    actionAddFlag->setText(tr("Add flag at %1").arg(RzAddressString(getLocationAddress())));
    actionAddFlag->setShortcutContext(Qt::ShortcutContext::WidgetWithChildrenShortcut);
    connect(actionAddFlag, &QAction::triggered, this, &HexWidget::onActionAddFlagTriggered);
    connect(this, &HexWidget::positionChanged, this, [this](RVA pos) {
        RzAnalysisFunction *fcn = Core()->functionAt(pos);
        if (fcn) {
            actionAddFlag->setVisible(false);
        } else {
            actionAddFlag->setVisible(true);
        }
    });
    addAction(actionAddFlag);

    // delete comment option
    actionDeleteComment = new QAction(tr("Delete Comment"), this);
    actionDeleteComment->setShortcutContext(Qt::ShortcutContext::WidgetWithChildrenShortcut);
    connect(actionDeleteComment, &QAction::triggered, this,
            &HexWidget::onActionDeleteCommentTriggered);
    addAction(actionDeleteComment);

    actionSelectRange = new QAction(tr("Select range"), this);
    connect(actionSelectRange, &QAction::triggered, this,
            [this]() { rangeDialog.open(cursor.address); });
    addAction(actionSelectRange);
    connect(&rangeDialog, &QDialog::accepted, this, &HexWidget::onRangeDialogAccepted);

    actionsWriteString.reserve(5);
    QAction *actionWriteString = new QAction(tr("Write string"), this);
    connect(actionWriteString, &QAction::triggered, this, &HexWidget::w_writeString);
    actionsWriteString.append(actionWriteString);

    QAction *actionWriteLenString = new QAction(tr("Write length and string"), this);
    connect(actionWriteLenString, &QAction::triggered, this, &HexWidget::w_writePascalString);
    actionsWriteString.append(actionWriteLenString);

    QAction *actionWriteWideString = new QAction(tr("Write wide string"), this);
    connect(actionWriteWideString, &QAction::triggered, this, &HexWidget::w_writeWideString);
    actionsWriteString.append(actionWriteWideString);

    QAction *actionWriteCString = new QAction(tr("Write zero terminated string"), this);
    connect(actionWriteCString, &QAction::triggered, this, &HexWidget::w_writeCString);
    actionsWriteString.append(actionWriteCString);

    QAction *actionWrite64 = new QAction(tr("Write a decoded or encoded Base64 string"), this);
    connect(actionWrite64, &QAction::triggered, this, &HexWidget::w_write64);
    actionsWriteString.append(actionWrite64);

    actionsWriteOther.reserve(5);
    QAction *actionWriteBytes = new QAction(tr("Write hex bytes"), this);
    connect(actionWriteBytes, &QAction::triggered, this, &HexWidget::w_writeBytes);
    actionsWriteOther.append(actionWriteBytes);

    QAction *actionWriteZeros = new QAction(tr("Write zeros"), this);
    connect(actionWriteZeros, &QAction::triggered, this, &HexWidget::w_writeZeros);
    actionsWriteOther.append(actionWriteZeros);

    QAction *actionWriteRandom = new QAction(tr("Write random bytes"), this);
    connect(actionWriteRandom, &QAction::triggered, this, &HexWidget::w_writeRandom);
    actionsWriteOther.append(actionWriteRandom);

    QAction *actionDuplicateFromOffset = new QAction(tr("Duplicate from offset"), this);
    connect(actionDuplicateFromOffset, &QAction::triggered, this, &HexWidget::w_duplFromOffset);
    actionsWriteOther.append(actionDuplicateFromOffset);

    QAction *actionIncDec = new QAction(tr("Increment/Decrement"), this);
    connect(actionIncDec, &QAction::triggered, this, &HexWidget::w_increaseDecrease);
    actionsWriteOther.append(actionIncDec);

    actionKeyboardEdit = new QAction(tr("Edit with keyboard"), this);
    actionKeyboardEdit->setCheckable(true);
    connect(actionKeyboardEdit, &QAction::triggered, this, &HexWidget::onKeyboardEditTriggered);
    connect(actionKeyboardEdit, &QAction::toggled, this, &HexWidget::onKeyboardEditChanged);

    connect(this, &HexWidget::selectionChanged, this,
            [this](Selection newSelection) { actionCopy->setEnabled(!newSelection.empty); });

    actionAddMark = Shortcuts()->makeAction("Hex.addMark", this);
    connect(actionAddMark, &QAction::triggered, this, &HexWidget::onActionAddMarkTriggered);
    addAction(actionAddMark);

    updateMetrics();
    updateItemLength();

    startAddress = 0ULL;
    cursor.address = 0ULL;
    data.reset(new MemoryData());
    oldData.reset(new MemoryData());

    fetchData();
    updateCursorMeta();

    connect(&cursor.blinkTimer, &QTimer::timeout, this, &HexWidget::onCursorBlinked);
    cursor.setBlinkPeriod(1000);
    cursor.startBlinking();

    updateColors();

    warningTimer.setSingleShot(true);
    connect(&warningTimer, &QTimer::timeout, this, &HexWidget::hideWarningRect);
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
    static const QVector<int> values({ 1, 2, 4, 8 });

    if (!values.contains(nbytes))
        return;

    finishEditingWord();

    itemByteLen = nbytes;
    if (itemByteLen > rowSizeBytes) {
        rowSizeBytes = itemByteLen;
    }

    actionsItemFormat.at(ItemFormatFloat)->setEnabled(nbytes >= 4);
    actionItemBigEndian->setEnabled(nbytes != 1);

    refreshWordEditState();

    updateItemLength();
    if (!cursorOnAscii && cursor.address % itemByteLen) {
        moveCursor(-int(cursor.address % itemByteLen));
    }
    fetchData();
    updateCursorMeta();

    viewport()->update();
}

void HexWidget::setItemFormat(ItemFormat format)
{
    finishEditingWord();

    itemFormat = format;

    bool sizeEnabled = true;
    if (format == ItemFormatFloat)
        sizeEnabled = false;
    actionsItemSize.at(0)->setEnabled(sizeEnabled);
    actionsItemSize.at(1)->setEnabled(sizeEnabled);

    refreshWordEditState();

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

/**
 * @brief Checks if Item at the address changed compared to the last read data.
 * @param address Address of Item to be compared.
 * @return True if Item is different, False if Item is equal or last read didn't contain the
 * address.
 * @see HexWidget#readItem
 *
 * Checks if current Item at the address changed compared to the last read data.
 * It is assumed that the current read data buffer contains the address.
 */
bool HexWidget::isItemDifferentAt(uint64_t address)
{
    char oldItem[sizeof(uint64_t)] = {};
    char newItem[sizeof(uint64_t)] = {};
    if (data->copy(newItem, address, static_cast<size_t>(itemByteLen))
        && oldData->copy(oldItem, address, static_cast<size_t>(itemByteLen))) {
        return memcmp(oldItem, newItem, sizeof(oldItem)) != 0;
    }
    return false;
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
    setCursorAddr(BasicCursor(cursor.address), false);
    emit selectionChanged(getSelection());
}

HexWidget::Selection HexWidget::getSelection()
{
    return Selection { selection.isEmpty(), selection.start(), selection.end() };
}

void HexWidget::seek(uint64_t address)
{
    if (!cursorOnAscii) {
        // when other widget causes seek to the middle of word
        // switch to ascii column which operates with byte positions
        auto viewOffset = startAddress % itemByteLen;
        auto addrOffset = address % itemByteLen;
        if ((addrOffset + itemByteLen - viewOffset) % itemByteLen) {
            setCursorOnAscii(true);
        }
    }
    setCursorAddr(BasicCursor(address));
}

void HexWidget::refresh()
{
    fetchData();
    viewport()->update();
}

void HexWidget::setItemEndianness(bool bigEndian)
{
    finishEditingWord();
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
    diffColor = Config()->getColor("graph.diff.unmatch");
    warningColor = QColor("red");

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

    if (event->rect() == cursor.screenPos.toAlignedRect()) {
        /* Cursor blink */
        drawCursor(painter);
        return;
    }

    painter.fillRect(event->rect().translated(xOffset, 0), backgroundColor);

    drawHeader(painter);

    drawAddrArea(painter);
    drawItemArea(painter);
    drawAsciiArea(painter);

    if (warningRectVisible) {
        painter.setPen(warningColor);
        painter.drawRect(warningRect);
    }

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

bool HexWidget::isFixedWidth() const
{
    return itemFormat == ItemFormatHex || itemFormat == ItemFormatOct;
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

    auto mouseAddr = mousePosToAddr(pos).address;

    QString infoText;
    if (!updatingSelection && (itemArea.contains(pos) || asciiArea.contains(pos))) {
        QString metaData = getFlagsAndComment(mouseAddr);
        if (!metaData.isEmpty() && itemArea.contains(pos)) {
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
            QColor c = mark.color;
            infoText += QString("<span style='white-space:nowrap; color: rgba(%1, %2, %3, %4);'>● "
                                "</span> %5")
                                .arg(c.red())
                                .arg(c.green())
                                .arg(c.blue())
                                .arg(MARK_ALPHA_F)
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
        bool holdingShift = event->modifiers() == Qt::ShiftModifier;

        // move cursor within actively edited item
        if (selectingData && !holdingShift && editWordState >= EditWordState::WriteNotEdited) {

            auto editWordArea = itemRectangle(cursor.address - startAddress);
            if (editWordArea.contains(pos)) {
                int wordOffset = 0;
                auto cursorPosition = screenPosToAddr(pos, false, &wordOffset);
                if (cursorPosition.address == cursor.address
                    // allow selecting after last character only when cursor limited to current word
                    && (wordOffset < editWord.length()
                        || navigationMode == HexNavigationMode::WordChar)) {
                    editWordPos = std::max(0, wordOffset);
                    editWordPos = std::min<int>(editWordPos, editWord.length());

                    if (isFixedWidth()) {
                        updatingSelection = true;
                        auto selectionCursor = cursorPosition;
                        if (editWordPos > itemCharLen / 2) {
                            selectionCursor += itemByteLen;
                        }
                        selection.init(selectionCursor);
                    }

                    viewport()->update();
                    return;
                }
            }
        }

        // cursor within any item if the mode allows
        if (selectingData && !holdingShift && editWordState >= EditWordState::WriteNotStarted
            && navigationMode == HexNavigationMode::AnyChar) {
            updatingSelection = true;
            setCursorOnAscii(false);
            int wordOffset = 0;
            auto cursorPosition = screenPosToAddr(pos, false, &wordOffset);
            finishEditingWord();
            if (isFixedWidth() && wordOffset >= itemCharLen - itemPrefixLen) {
                wordOffset = 0;
                cursorPosition += itemByteLen;
            }
            setCursorAddr(cursorPosition, holdingShift);
            auto selectionPosition = currentAreaPosToAddr(pos, true);
            selection.init(selectionPosition);
            emit selectionChanged(getSelection());

            if (wordOffset > 0) {
                startEditWord();
                editWordPos = std::min<int>(wordOffset, editWord.length() - 1);
            }
            viewport()->update();
            return;
        }

        if (selecting) {
            finishEditingWord();

            updatingSelection = true;
            setCursorOnAscii(!selectingData);
            auto cursorPosition = currentAreaPosToAddr(pos, true);
            setCursorAddr(cursorPosition, holdingShift);
            viewport()->update();
        }
    }
}

void HexWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    QPoint pos(event->pos());
    pos.rx() += horizontalScrollBar()->value();

    if (event->button() == Qt::LeftButton && !isFixedWidth()
        && editWordState == EditWordState::WriteNotStarted && itemArea.contains(pos)) {
        int wordOffset = 0;
        auto cursorPosition = screenPosToAddr(pos, false, &wordOffset);
        setCursorAddr(cursorPosition, false);
        startEditWord();
        int padding = std::max<int>(0, itemCharLen - editWord.length());
        editWordPos = std::max(0, wordOffset - padding);
        editWordPos = std::min<int>(editWordPos, editWord.length());
    }
}

void HexWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (selection.isEmpty()) {
            selection.init(BasicCursor(cursor.address));
            cursorEnabled = true;
            viewport()->update();
        }
        updatingSelection = false;
    }
}

void HexWidget::wheelEvent(QWheelEvent *event)
{
    // according to Qt doc 1 row per 5 degrees, angle measured in 1/8 of degree
    int dy = event->angleDelta().y() / (8 * 5);
    int64_t delta = -dy * itemRowByteLen();

    if (dy == 0)
        return;

    if (delta < 0 && startAddress < static_cast<uint64_t>(-delta)) {
        startAddress = 0;
    } else if (delta > 0 && data->maxIndex() < static_cast<uint64_t>(bytesPerScreen())) {
        startAddress = 0;
    } else if ((data->maxIndex() - startAddress)
               <= static_cast<uint64_t>(bytesPerScreen() + delta - 1)) {
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

bool HexWidget::validCharForEdit(QChar digit)
{
    switch (itemFormat) {
    case ItemFormatHex:
        return (digit >= '0' && digit <= '9') || (digit >= 'a' && digit <= 'f')
                || (digit >= 'A' && digit <= 'F');
    case ItemFormatOct: {
        if (editWordPos > 0) {
            return (digit >= '0' && digit <= '7');
        } else {
            int bitsInMSD = (itemByteLen * 8) % 3;
            int biggestDigit = (1 << bitsInMSD) - 1;
            return digit >= '0' && digit <= char('0' + biggestDigit);
        }
    }
    case ItemFormatDec:
        return (digit >= '0' && digit <= '9');
    case ItemFormatSignedDec:
        return (digit >= '0' && digit <= '9') || digit == '-';
    case ItemFormatFloat:
        return (digit >= '0' && digit <= '9') || digit == '-' || digit == '+' || digit == '.'
                || digit == ',' || digit == '+' || digit == 'e' || digit == 'E' || digit == 'i'
                || digit == 'n' || digit == 'f' || digit == 'I' || digit == 'N' || digit == 'F'
                || digit == 'a' || digit == 'A';
    }

    return false;
}

void HexWidget::movePrevEditCharAny()
{
    if (!selection.isEmpty()) {
        clearSelection();
    }
    editWordPos -= 1;
    if (editWordPos < 0) {
        finishEditingWord();
        if (moveCursor(-itemByteLen, false, OverflowMove::Ignore)) {
            startEditWord();
            editWordPos = editWord.length() - 1;
        }
    }
    viewport()->update();
}

void HexWidget::typeOverwriteModeChar(QChar c)
{
    if (editWordState < EditWordState::WriteNotEdited || !isFixedWidth()) {
        return;
    }
    editWord[editWordPos] = c;
    editWordPos++;
    editWordState = EditWordState::WriteEdited;
    if (editWordPos >= editWord.length()) {
        finishEditingWord();
        bool moved = moveCursor(itemByteLen, false, OverflowMove::Ignore);
        startEditWord();
        if (!moved) {
            editWordPos = editWord.length() - 1;
        }
    }
}

HexWidget::HexNavigationMode HexWidget::defaultNavigationMode()
{
    switch (editWordState) {
    case EditWordState::Read:
        return HexNavigationMode::Words;
    case EditWordState::WriteNotStarted:
        return isFixedWidth() ? HexNavigationMode::AnyChar : HexNavigationMode::Words;
    case EditWordState::WriteNotEdited:
    case EditWordState::WriteEdited:
        return isFixedWidth() ? HexNavigationMode::AnyChar : HexNavigationMode::WordChar;
    }
    return HexNavigationMode::Words;
}

void HexWidget::refreshWordEditState()
{
    navigationMode = defaultNavigationMode();
}

bool HexWidget::handleAsciiWrite(QKeyEvent *event)
{
    if (!cursorOnAscii || !canKeyboardEdit()) {
        return false;
    }
    if (event->key() == Qt::Key_Backspace || event->matches(QKeySequence::Backspace)) {
        if (!selection.isEmpty()) {
            writeZeros(selection.start(), selection.size());
        } else {
            moveCursor(-1, false);
            writeZeros(cursor.address, 1);
        }
        return true;
    }
    if (event->key() == Qt::Key_Delete || event->matches(QKeySequence::Delete)) {
        if (!selection.isEmpty()) {
            writeZeros(selection.start(), selection.size());
        } else {
            writeZeros(cursor.address, 1);
            moveCursor(1, false);
        }
        return true;
    }
    QString text;
    if (event->matches(QKeySequence::Paste)) {
        text = QApplication::clipboard()->text();
        if (text.length() <= 0) {
            return false;
        }
    } else {
        text = event->text();
        if (text.length() <= 0) {
            return false;
        }
        QChar c = text[0];
        if (c <= '\x1f' || c == '\x7f') {
            return false;
        }
    }

    auto bytes = text.toUtf8(); // TODO:#3028 use selected text encoding
    auto address = getLocationAddress();
    clearSelection();
    data->write(reinterpret_cast<const uint8_t *>(bytes.data()), address, bytes.length());
    seek(address + bytes.length());
    viewport()->update();
    return true;
}

bool HexWidget::handleNumberWrite(QKeyEvent *event)
{
    if (editWordState < EditWordState::WriteNotStarted) {
        return false;
    }
    bool overwrite = isFixedWidth();
    auto keyText = event->text();
    bool editingWord = editWordState >= EditWordState::WriteNotEdited;
    if (keyText.length() > 0 && validCharForEdit(keyText[0])) {
        if (!selection.isEmpty()) {
            setCursorAddr(BasicCursor(selection.start()));
        }
        if (!editingWord) {
            startEditWord();
        }
        if (overwrite) {
            typeOverwriteModeChar(keyText[0]);
        } else if (!editingWord /* && !overwrite */) {
            editWord = keyText;
            editWordPos = editWord.length();
            editWordState = EditWordState::WriteEdited;
        } else if (itemFormat == ItemFormatFloat || editWord.length() < itemCharLen) {
            editWord.insert(editWordPos, keyText);
            editWordPos += keyText.length();
            editWordState = EditWordState::WriteEdited;
        }
        maybeFlushCharEdit();
        return true;
    }
    if (event->matches(QKeySequence::Paste) && (editingWord || overwrite)) {
        QString text = QApplication::clipboard()->text();
        if (text.length() > 0) {
            if (overwrite) {
                startEditWord();
                for (QChar c : text) {
                    if (validCharForEdit(c)) {
                        typeOverwriteModeChar(c);
                    }
                }
            } else {
                editWord.insert(editWordPos, text);
                editWordPos += text.length();
                editWordState = EditWordState::WriteEdited;
            }
        }
        maybeFlushCharEdit();
        return true;
    }
    if (editingWord) {
        if (event->matches(QKeySequence::Cancel)) {
            cancelEditedWord();
            return true;
        } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            bool needToAdvance =
                    !(editWordPos == 0 && overwrite && editWordState < EditWordState::WriteEdited);
            if (finishEditingWord(false) && needToAdvance) {
                moveCursor(itemByteLen);
            }
            return true;
        }
    }
    if (event->matches(QKeySequence::Delete)) {
        if (!selection.isEmpty()) {
            writeZeros(selection.start(), selection.size());
            clearSelection();
        } else {
            startEditWord();
            if (overwrite) {
                typeOverwriteModeChar('0');
            } else if (editWordPos < editWord.length()) {
                editWord.remove(editWordPos, 1);
                editWordState = EditWordState::WriteEdited;
            }
        }
        maybeFlushCharEdit();
        return true;
    }
    if (event->matches(QKeySequence::DeleteEndOfWord) && selection.isEmpty()) {
        startEditWord();
        if (overwrite) {
            for (int i = editWordPos; i < editWord.length(); i++) {
                typeOverwriteModeChar('0');
            }
        } else if (editWordPos < editWord.length()) {
            editWord.remove(editWordPos, editWord.length());
            editWordState = EditWordState::WriteEdited;
        }
        maybeFlushCharEdit();
        return true;
    }
    if (event->matches(QKeySequence::DeleteStartOfWord) && selection.isEmpty()) {
        if (!editingWord || (overwrite && editWordPos == 0)) {
            if (!moveCursor(-itemByteLen, false, OverflowMove::Ignore)) {
                return false;
            }
            startEditWord();
            editWordPos = editWord.length();
        }
        if (overwrite) {
            while (editWordPos > 0) {
                editWordPos--;
                editWord[editWordPos] = '0';
            }
            editWordState = EditWordState::WriteEdited;
            maybeFlushCharEdit();
            return true;
        } else {
            if (editWordPos > 0) {
                editWord.remove(0, editWordPos);
                editWordState = EditWordState::WriteEdited;
                editWordPos = 0;
                maybeFlushCharEdit();
                return true;
            }
        }
        return false;
    }

    if (event->key() == Qt::Key_Backspace) {
        if (!selection.isEmpty()) {
            writeZeros(selection.start(), selection.size());
            clearSelection();
            setCursorAddr(BasicCursor(selection.start()), false);
            return true;
        } else {
            if (!editingWord || (overwrite && editWordPos == 0)) {
                if (!moveCursor(-itemByteLen, false, OverflowMove::Ignore)) {
                    return false;
                }
                startEditWord();
                editWordPos = editWord.length();
            }
            if (editWordPos > 0) {
                editWordPos -= 1;
                if (overwrite) {
                    editWord[editWordPos] = '0';
                } else {
                    editWord.remove(editWordPos, 1);
                }
                editWordState = EditWordState::WriteEdited;
                maybeFlushCharEdit();
                return true;
            }
        }
        return false;
    }

    return false;
}

bool HexWidget::event(QEvent *event)
{
    // prefer treating keys like 's' 'g' '.' as typing input instead of global shortcuts
    if (event->type() == QEvent::ShortcutOverride) {
        auto keyEvent = static_cast<QKeyEvent *>(event);
        auto modifiers = keyEvent->modifiers();
        if ((modifiers == Qt::NoModifier || modifiers == Qt::ShiftModifier
             || modifiers == Qt::KeypadModifier)
            && keyEvent->key() < Qt::Key_Escape && canKeyboardEdit()) {
            keyEvent->accept();
            return true;
        }
    }

    return QScrollArea::event(event);
}

void HexWidget::keyPressEvent(QKeyEvent *event)
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

    if (canKeyboardEdit()) {
        if (handleAsciiWrite(event)) {
            viewport()->update();
            return;
        }
        if (editWordState >= EditWordState::WriteNotStarted && !cursorOnAscii) {
            if (handleNumberWrite(event)) {
                viewport()->update();
                return;
            }
        }
    }

    if (cursorOnAscii || navigationMode == HexNavigationMode::Words
        || navigationMode == HexNavigationMode::AnyChar) {
        if (moveOrSelect(QKeySequence::MoveToNextPage, QKeySequence::SelectNextPage)) {
            moveCursor(bytesPerScreen(), select);
        } else if (moveOrSelect(QKeySequence::MoveToPreviousPage,
                                QKeySequence::SelectPreviousPage)) {
            moveCursor(-bytesPerScreen(), select);
        } else if (moveOrSelect(QKeySequence::MoveToStartOfLine, QKeySequence::SelectStartOfLine)) {
            int linePos =
                    int((cursor.address % itemRowByteLen()) - (startAddress % itemRowByteLen()));
            moveCursor(-linePos, select);
        } else if (moveOrSelect(QKeySequence::MoveToEndOfLine, QKeySequence::SelectEndOfLine)) {
            int linePos =
                    int((cursor.address % itemRowByteLen()) - (startAddress % itemRowByteLen()));
            moveCursor(itemRowByteLen() - linePos, select);
        }
    }

    if (navigationMode == HexNavigationMode::Words || cursorOnAscii) {
        if (moveOrSelect(QKeySequence::MoveToNextLine, QKeySequence::SelectNextLine)) {
            moveCursor(itemRowByteLen(), select, OverflowMove::Ignore);
        } else if (moveOrSelect(QKeySequence::MoveToPreviousLine,
                                QKeySequence::SelectPreviousLine)) {
            moveCursor(-itemRowByteLen(), select, OverflowMove::Ignore);
        } else if (moveOrSelect(QKeySequence::MoveToNextChar, QKeySequence::SelectNextChar)
                   || moveOrSelect(QKeySequence::MoveToNextWord, QKeySequence::SelectNextWord)) {
            moveCursor(cursorOnAscii ? 1 : itemByteLen, select);
        } else if (moveOrSelect(QKeySequence::MoveToPreviousChar, QKeySequence::SelectPreviousChar)
                   || moveOrSelect(QKeySequence::MoveToPreviousWord,
                                   QKeySequence::SelectPreviousWord)) {
            moveCursor(cursorOnAscii ? -1 : -itemByteLen, select);
        }
    } else if (navigationMode == HexNavigationMode::AnyChar && !cursorOnAscii) {
        if (moveOrSelect(QKeySequence::MoveToNextLine, QKeySequence::SelectNextLine)) {
            moveCursorKeepEditOffset(itemRowByteLen(), select, OverflowMove::Ignore);
        } else if (moveOrSelect(QKeySequence::MoveToPreviousLine,
                                QKeySequence::SelectPreviousLine)) {
            moveCursorKeepEditOffset(-itemRowByteLen(), select, OverflowMove::Ignore);
        } else if (moveOrSelect(QKeySequence::MoveToNextChar, QKeySequence::SelectNextChar)) {
            if (select) {
                moveCursor(itemByteLen, select);
            } else {
                if (!selection.isEmpty()) {
                    clearSelection();
                }
                if (editWordState == EditWordState::WriteNotStarted) {
                    startEditWord();
                }
                editWordPos += 1;
                if (editWordPos >= editWord.length()) {
                    bool moved = moveCursor(itemByteLen, false, OverflowMove::Ignore);
                    startEditWord();
                    if (!moved) {
                        editWordPos = editWord.length() - 1;
                    }
                }
            }
            viewport()->update();
        } else if (event->matches(QKeySequence::MoveToPreviousChar)) {
            movePrevEditCharAny();
        } else if (event->matches(QKeySequence::SelectPreviousChar)) {
            moveCursor(-itemByteLen, true);
        } else if (moveOrSelect(QKeySequence::MoveToNextWord, QKeySequence::SelectNextWord)) {
            moveCursor(itemByteLen, select);
        } else if (event->matches(QKeySequence::MoveToPreviousWord)) {
            if (editWordPos > 0) {
                editWordPos = 0;
                viewport()->update();
            } else {
                moveCursor(-itemByteLen, false);
            }
        } else if (event->matches(QKeySequence::SelectPreviousWord)) {
            moveCursor(-itemByteLen, true);
        }
    } else if (navigationMode == HexNavigationMode::WordChar) {
        if (event->matches(QKeySequence::MoveToNextChar)) {
            editWordPos = std::min<int>(editWord.length(), editWordPos + 1);
            viewport()->update();
        } else if (event->matches(QKeySequence::MoveToPreviousChar)) {
            editWordPos = std::max(0, editWordPos - 1);
            viewport()->update();
        } else if (event->matches(QKeySequence::MoveToStartOfLine)) {
            editWordPos = 0;
            viewport()->update();
        } else if (event->matches(QKeySequence::MoveToEndOfLine)) {
            editWordPos = editWord.length();
            viewport()->update();
        } else if (event->matches(QKeySequence::MoveToPreviousWord)) {
            if (editWordPos > 0) {
                editWordPos = 0;
            } else {
                moveCursor(-itemByteLen, select);
                startEditWord();
            }
            viewport()->update();
        } else if (event->matches(QKeySequence::MoveToNextWord)) {
            if (editWordPos < editWord.length()) {
                editWordPos = editWord.length();
            } else {
                moveCursor(itemByteLen, select);
                startEditWord();
                editWordPos = editWord.length();
            }
            viewport()->update();
        }
    }
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

    QString comment = Core()->getCommentAt(cursor.address);

    if (comment.isEmpty()) {
        actionDeleteComment->setVisible(false);
        actionComment->setText(tr("Add Comment"));
    } else {
        actionDeleteComment->setVisible(true);
        actionComment->setText(tr("Edit Comment"));
    }

    QString flag = Core()->flagAt(cursor.address, false);
    actionAddFlag->setData(flag);

    if (flag.isEmpty()) {
        actionAddFlag->setText(tr("Add flag at %1").arg(RzAddressString(cursor.address)));
    } else {
        actionAddFlag->setText(tr("Rename flag \"%1\"").arg(flag));
    }

    if (!ioModesController.canWrite()) {
        actionKeyboardEdit->setChecked(false);
    }

    auto *menu = new QMenu(this);
    QMenu *sizeMenu = menu->addMenu(tr("Item size:"));
    sizeMenu->addActions(actionsItemSize);
    QMenu *formatMenu = menu->addMenu(tr("Item format:"));
    formatMenu->addActions(actionsItemFormat);
    menu->addMenu(rowSizeMenu);
    menu->addAction(actionHexPairs);
    menu->addAction(actionItemBigEndian);
    QMenu *writeMenu = menu->addMenu(tr("Edit"));
    writeMenu->addActions(actionsWriteString);
    writeMenu->addSeparator();
    writeMenu->addActions(actionsWriteOther);
    menu->addAction(actionKeyboardEdit);

    menu->addSeparator();
    menu->addAction(actionCopy);
    disableOutsideSelectionActions(mouseOutsideSelection);
    menu->addAction(actionCopyAddress);
    menu->addActions(this->actions());

    /* Marks */
    menu->addSeparator();
    menu->addAction(actionAddMark);
    auto marks = Core()->getMarksAt(cursor.address);
    if (!marks.empty()) {
        if (marks.size() == 1) {
            // Add direct actions if only one mark contains this address
            const auto &mark = marks.front();
            QAction *editAction = menu->addAction(tr("Edit Mark"));
            QAction *removeAction = menu->addAction(tr("Remove Mark"));
            QString markName = mark.name;
            connect(removeAction, &QAction::triggered, this,
                    [this, markName]() { onActionDeleteMarkTriggered(markName); });
            connect(editAction, &QAction::triggered, this,
                    [this, markName]() { onActionEditMarkTriggered(markName); });
        } else {
            // Add submenus if multiple marks contain this address
            QMenu *editMarkMenu = menu->addMenu(tr("Edit Mark"));
            QMenu *removeMarkMenu = menu->addMenu(tr("Remove Mark"));
            for (const auto &mark : marks) {
                QAction *subActionRename = removeMarkMenu->addAction(mark.realname);
                QAction *subActionEdit = editMarkMenu->addAction(mark.realname);
                QString markName = mark.name;
                connect(subActionRename, &QAction::triggered, this,
                        [this, markName]() { onActionDeleteMarkTriggered(markName); });
                connect(subActionEdit, &QAction::triggered, this,
                        [this, markName]() { onActionEditMarkTriggered(markName); });
            }
        }
    }

    menu->exec(mapToGlobal(pt));
    disableOutsideSelectionActions(false);
    menu->deleteLater();
}

void HexWidget::onCursorBlinked()
{
    if (!cursorEnabled)
        return;
    cursor.blink();
    QRect cursorRect = cursor.screenPos.toAlignedRect();
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

    auto x = cursorOnAscii
            ? Core()->getString(selection.start(), selection.size(), RZ_STRING_ENC_8BIT, true)
            : Core()->ioRead(selection.start(), (int)selection.size()).toHex();
    QApplication::clipboard()->setText(x);
}

void HexWidget::copyAddress()
{
    uint64_t addr = getLocationAddress();
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(RzAddressString(addr));
}

// slot for add comment action
void HexWidget::onActionAddCommentTriggered()
{
    uint64_t addr = cursor.address;
    CommentsDialog::addOrEditComment(addr, this);
    refresh();
}

// slot for deleting comment action
void HexWidget::onActionDeleteCommentTriggered()
{
    uint64_t addr = cursor.address;
    Core()->delComment(addr);
    refresh();
}

void HexWidget::onActionAddFlagTriggered()
{
    QString flagNameHint = actionAddFlag->data().toString();
    if (FlagDialog(cursor.address, this, flagNameHint).exec()) {
        refresh();
    }
}

void HexWidget::onRangeDialogAccepted()
{
    if (rangeDialog.empty()) {
        seek(rangeDialog.getStartAddress());
        return;
    }
    selectRange(rangeDialog.getStartAddress(), rangeDialog.getEndAddress());
}

void HexWidget::onActionAddMarkTriggered()
{
    RVA from = cursor.address;
    RVA to = cursor.address;
    if (!selection.isEmpty()) {
        from = selection.start();
        to = selection.end();
    }
    if (MarkDialog(from, to, this).exec()) {
        refresh();
    }
}

void HexWidget::onActionDeleteMarkTriggered(const QString &name)
{
    Core()->delMark(name);
    refresh();
}

void HexWidget::onActionEditMarkTriggered(const QString &name)
{
    RVA addr = cursor.address;
    if (MarkDialog(addr, addr, this, name).exec()) {
        refresh();
    }
}

void HexWidget::writeZeros(uint64_t address, uint64_t length)
{
    const uint64_t MAX_BUFFER = 1024;
    std::vector<uint8_t> zeroes(std::min(MAX_BUFFER, length), 0);
    while (length > zeroes.size()) {
        data->write(zeroes.data(), address, zeroes.size());
        address += zeroes.size();
        length -= zeroes.size();
    }
    if (length > 0) {
        data->write(zeroes.data(), address, length);
    }
}

void HexWidget::w_writeString()
{
    if (!ioModesController.prepareForWriting()) {
        return;
    }
    bool ok = false;
    QString str = QInputDialog::getText(this, tr("Write string"), tr("String:"), QLineEdit::Normal,
                                        "", &ok);
    if (!ok || str.isEmpty()) {
        return;
    }
    {
        RzCoreLocked core(Core());
        rz_core_write_string_at(core, getLocationAddress(), str.toUtf8().constData());
    }
    refresh();
}

void HexWidget::w_increaseDecrease()
{
    if (!ioModesController.prepareForWriting()) {
        return;
    }
    IncrementDecrementDialog d;
    int ret = d.exec();
    if (ret == QDialog::Rejected) {
        return;
    }
    int64_t value = (int64_t)d.getValue();
    uint8_t sz = d.getNBytes();
    if (d.getMode() == IncrementDecrementDialog::Decrease) {
        value *= -1;
    }
    {
        RzCoreLocked core(Core());
        rz_core_write_value_inc_at(core, getLocationAddress(), value, sz);
    }
    refresh();
}

void HexWidget::w_writeBytes()
{
    if (!ioModesController.prepareForWriting()) {
        return;
    }
    bool ok = false;

    int size = INT_MAX;
    if (!selection.isEmpty() && selection.size() <= INT_MAX) {
        size = static_cast<int>(selection.size());
    }

    QByteArray bytes = QInputDialog::getText(this, tr("Write hex bytes"), tr("Hex byte string:"),
                                             QLineEdit::Normal, "", &ok)
                               .toUtf8();
    const int offset = bytes.startsWith("\\x") ? 2 : 0;
    const int incr = offset + 2;
    const int bytes_size = qMin(bytes.size() / incr, size);
    if (!ok || !bytes_size) {
        return;
    }
    {
        auto *buf = (uint8_t *)malloc(static_cast<size_t>(bytes_size));
        if (!buf) {
            return;
        }
        for (int i = 0, j = 0, sz = bytes.size(); i < sz; i += incr, j++) {
            buf[j] = static_cast<uint8_t>(bytes.mid(i + offset, 2).toInt(nullptr, 16));
        }
        RzCoreLocked core(Core());
        rz_core_write_at(core, getLocationAddress(), buf, bytes_size);
        free(buf);
    }
    refresh();
}

void HexWidget::w_writeZeros()
{
    if (!ioModesController.prepareForWriting()) {
        return;
    }

    int size = 1;
    if (!selection.isEmpty() && selection.size() <= INT_MAX) {
        size = static_cast<int>(selection.size());
    }

    bool ok = false;
    int len = QInputDialog::getInt(this, tr("Write zeros"), tr("Number of zeros:"), size, 1,
                                   0x7FFFFFFF, 1, &ok);
    if (!ok) {
        return;
    }
    {
        writeZeros(getLocationAddress(), len);
    }
    refresh();
}

void HexWidget::w_write64()
{
    if (!ioModesController.prepareForWriting()) {
        return;
    }
    Base64EnDecodedWriteDialog d;
    int ret = d.exec();
    if (ret == QDialog::Rejected) {
        return;
    }
    QByteArray str = d.getData();

    if (d.getMode() == Base64EnDecodedWriteDialog::Decode
        && (QString(str).contains(QRegularExpression("[^a-zA-Z0-9+/=]")) || str.length() % 4 != 0
            || str.isEmpty())) {
        QMessageBox::critical(
                this, tr("Error"),
                tr("Error occured during decoding your input.\n"
                   "Please, make sure, that it is a valid base64 string and try again."));
        return;
    }

    {
        RzCoreLocked core(Core());
        if (d.getMode() == Base64EnDecodedWriteDialog::Encode) {
            rz_core_write_base64_at(core, getLocationAddress(), str.toHex().constData());
        } else {
            rz_core_write_base64d_at(core, getLocationAddress(), str.constData());
        }
    }
    refresh();
}

void HexWidget::w_writeRandom()
{
    if (!ioModesController.prepareForWriting()) {
        return;
    }

    int size = 1;
    if (!selection.isEmpty() && selection.size() <= INT_MAX) {
        size = static_cast<int>(selection.size());
    }

    bool ok = false;
    int nbytes = QInputDialog::getInt(this, tr("Write random bytes"), tr("Number of bytes:"), size,
                                      1, 0x7FFFFFFF, 1, &ok);
    if (!ok) {
        return;
    }

    {
        RzCoreLocked core(Core());
        rz_core_write_random_at(core, getLocationAddress(), nbytes);
    }
    refresh();
}

void HexWidget::w_duplFromOffset()
{
    if (!ioModesController.prepareForWriting()) {
        return;
    }
    DuplicateFromOffsetDialog d;
    int ret = d.exec();
    if (ret == QDialog::Rejected) {
        return;
    }
    RVA src = d.getOffset();
    int len = (int)d.getNBytes();
    {
        RzCoreLocked core(Core());
        rz_core_write_duplicate_at(core, getLocationAddress(), src, len);
    }
    refresh();
}

void HexWidget::w_writePascalString()
{
    if (!ioModesController.prepareForWriting()) {
        return;
    }
    bool ok = false;
    QString str = QInputDialog::getText(this, tr("Write Pascal string"), tr("String:"),
                                        QLineEdit::Normal, "", &ok);
    if (!ok || str.isEmpty()) {
        return;
    }
    {
        RzCoreLocked core(Core());
        rz_core_write_length_string_at(core, getLocationAddress(), str.toUtf8().constData());
    }
    refresh();
}

void HexWidget::w_writeWideString()
{
    if (!ioModesController.prepareForWriting()) {
        return;
    }
    bool ok = false;
    QString str = QInputDialog::getText(this, tr("Write wide string"), tr("String:"),
                                        QLineEdit::Normal, "", &ok);
    if (!ok || str.isEmpty()) {
        return;
    }
    {
        RzCoreLocked core(Core());
        rz_core_write_string_wide_at(core, getLocationAddress(), str.toUtf8().constData());
    }
    refresh();
}

void HexWidget::w_writeCString()
{
    if (!ioModesController.prepareForWriting()) {
        return;
    }
    bool ok = false;
    QString str = QInputDialog::getText(this, tr("Write zero-terminated string"), tr("String:"),
                                        QLineEdit::Normal, "", &ok);
    if (!ok || str.isEmpty()) {
        return;
    }
    {
        RzCoreLocked core(Core());
        rz_core_write_string_zero_at(core, getLocationAddress(), str.toUtf8().constData());
    }
    refresh();
}

void HexWidget::onKeyboardEditTriggered(bool enabled)
{
    if (!enabled) {
        return;
    }
    if (!ioModesController.prepareForWriting()) {
        actionKeyboardEdit->setChecked(false);
    }
}

void HexWidget::onKeyboardEditChanged(bool enabled)
{
    if (!enabled) {
        finishEditingWord();
        navigationMode = HexNavigationMode::Words;
        editWordState = EditWordState::Read;
    } else {
        editWordState = EditWordState::WriteNotStarted;
        navigationMode = defaultNavigationMode();
    }
    updateCursorMeta();
    viewport()->update();
}

void HexWidget::updateItemLength()
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
            painter.drawText(rect, Qt::AlignVCenter | Qt::AlignRight,
                             QString::number(offset, 16).toUpper());
            rect.translate(itemWidth(), 0);
        }
        rect.translate(columnSpacingWidth(), 0);
    }

    rect.moveLeft(asciiArea.left());
    rect.setWidth(charWidth);
    for (int j = 0; j < itemRowByteLen(); ++j) {
        painter.drawText(rect, Qt::AlignVCenter | Qt::AlignRight,
                         QString::number(j % 16, 16).toUpper());
        rect.translate(charWidth, 0);
    }
}

void HexWidget::drawCursor(QPainter &painter, bool shadow)
{
    if (shadow) {
        QPen pen(Qt::gray);
        pen.setStyle(Qt::DashLine);
        painter.setPen(pen);
        qreal shadowWidth = charWidth;
        if (cursorOnAscii) {
            shadowWidth = itemWidth();
        } else if (editWordState >= EditWordState::WriteNotEdited) {
            shadowWidth = itemByteLen * charWidth;
        }
        shadowCursor.screenPos.setWidth(shadowWidth);
        painter.drawRect(shadowCursor.screenPos);
        painter.setPen(Qt::SolidLine);
    }

    painter.setPen(cursor.cachedColor);
    QRectF charRect(cursor.screenPos);
    charRect.setWidth(charWidth);
    painter.fillRect(charRect, backgroundColor);
    QColor markColor = Core()->getBlendedMarksColorAt(cursor.address);
    if (markColor.isValid()) {
        painter.fillRect(charRect, markColor);
    }
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
    for (int line = 0; line < visibleLines && offset <= data->maxIndex();
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

void HexWidget::fillMarks(QPainter &painter, bool ascii)
{
    uint64_t endAddress = startAddress + visibleLines * itemColumns * itemGroupSize * itemByteLen;
    endAddress = std::min(endAddress, data->maxIndex());
    const auto marks = Core()->getMarks();
    for (const auto &mark : marks) {
        ut64 from = mark.from;
        ut64 to = mark.to;

        // Skip marks not visible in current viewport
        if (to < startAddress || from > endAddress) {
            continue;
        }

        const auto shapes = rangePolygons(from, to, ascii);

        QColor color(mark.color);
        if (!color.isValid()) {
            continue;
        }
        color.setAlphaF(MARK_ALPHA_F);
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);

        for (const auto &shape : shapes) {
            painter.drawPolygon(shape);
        }
    }
}

void HexWidget::drawItemArea(QPainter &painter)
{
    QRectF itemRect(itemArea.topLeft(), QSizeF(itemWidth(), lineHeight));
    QColor itemColor;
    QString itemString;

    fillSelectionBackground(painter);
    fillMarks(painter, false);

    bool haveEditWord = false;
    QRectF editWordRect;
    QColor editWordColor;

    uint64_t itemAddr = startAddress;
    for (int line = 0; line < visibleLines; ++line) {
        itemRect.moveLeft(itemArea.left());
        for (int j = 0; j < itemColumns; ++j) {
            for (int k = 0; k < itemGroupSize && itemAddr <= data->maxIndex();
                 ++k, itemAddr += itemByteLen) {

                itemString = renderItem(itemAddr - startAddress, &itemColor);

                if (!getFlagsAndComment(itemAddr).isEmpty()) {
                    QColor markerColor(borderColor);
                    markerColor.setAlphaF(0.5);
                    painter.setPen(markerColor);
                    for (const auto &shape : rangePolygons(itemAddr, itemAddr, false)) {
                        painter.drawPolyline(shape);
                    }
                }
                if (selection.contains(itemAddr) && !cursorOnAscii) {
                    itemColor = palette().highlightedText().color();
                }
                if (isItemDifferentAt(itemAddr)) {
                    itemColor.setRgb(diffColor.rgb());
                }

                if (editWordState <= EditWordState::WriteNotStarted || cursor.address != itemAddr) {
                    painter.setPen(itemColor);
                    painter.drawText(itemRect, Qt::AlignVCenter, itemString);
                    itemRect.translate(itemWidth(), 0);
                    if (cursor.address == itemAddr) {
                        auto &itemCursor = cursorOnAscii ? shadowCursor : cursor;
                        int itemCharPos = 0;
                        if (editWordState > EditWordState::Read) {
                            itemCharPos += itemPrefixLen;
                        }
                        if (itemCharPos < itemString.length()) {
                            itemCursor.cachedChar = itemString.at(itemCharPos);
                        } else {
                            itemCursor.cachedChar = ' ';
                        }
                        itemCursor.cachedColor = itemColor;
                    }
                } else {
                    haveEditWord = true;
                    editWordRect = itemRect;
                    editWordColor = itemColor;

                    auto &itemCursor = cursor;
                    itemCursor.cachedChar =
                            editWordPos < editWord.length() ? editWord[editWordPos] : QChar(' ');
                    itemCursor.cachedColor = itemColor;
                    itemCursor.screenPos.moveTopLeft(itemRect.topLeft());
                    itemCursor.screenPos.translate(charWidth * (editWordPos + itemPrefixLen), 0);

                    itemRect.translate(itemWidth(), 0);
                }
            }
            itemRect.translate(columnSpacingWidth(), 0);
        }
        itemRect.translate(0, lineHeight);
    }
    if (haveEditWord) {
        auto length = std::max<int>(itemCharLen, editWord.length());
        auto rect = editWordRect;
        rect.setWidth(length * charWidth);
        painter.fillRect(rect, backgroundColor);

        painter.setPen(editWordColor);
        editWordRect.setWidth(4000);
        painter.drawText(editWordRect, Qt::AlignVCenter | Qt::AlignLeft, itemPrefix + editWord);
    }

    painter.setPen(borderColor);

    qreal vLineOffset = asciiArea.left() - charWidth;
    painter.drawLine(QLineF(vLineOffset, 0, vLineOffset, viewport()->height()));
}

void HexWidget::drawAsciiArea(QPainter &painter)
{
    QRectF charRect(asciiArea.topLeft(), QSizeF(charWidth, lineHeight));

    fillSelectionBackground(painter, true);
    fillMarks(painter, true);
    painter.setBrush(Qt::NoBrush);

    uint64_t address = startAddress;
    QChar ascii;
    QColor color;
    for (int line = 0; line < visibleLines; ++line, charRect.translate(0, lineHeight)) {
        charRect.moveLeft(asciiArea.left());
        for (int j = 0; j < itemRowByteLen() && address <= data->maxIndex(); ++j, ++address) {
            ascii = renderAscii(address - startAddress, &color);
            if (selection.contains(address) && cursorOnAscii) {
                color = palette().highlightedText().color();
            }
            if (isItemDifferentAt(address)) {
                color.setRgb(diffColor.rgb());
            }
            painter.setPen(color);
            /* Dots look ugly. Use fillRect() instead of drawText(). */
            if (ascii == '.') {
                qreal a = cursor.screenPos.width();
                QPointF p = charRect.bottomLeft();
                p.rx() += (charWidth - a) / 2 + 1;
                p.ry() += -2 * a;
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
    for (const auto &shape : parts) {
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
    bool startJagged = false;
    bool endJagged = false;
    if (!ascii) {
        if (int startFraction = startOffset % itemByteLen) {
            startRect.setLeft(startRect.left() + startFraction * startRect.width() / itemByteLen);
            startJagged = true;
        }
        if (int endFraction = itemByteLen - 1 - (endOffset % itemByteLen)) {
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
        QPointF adjustment(charWidth / 16, 0);
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

void HexWidget::updateMetrics()
{
    QFontMetricsF fontMetrics(this->monospaceFont);
    lineHeight = fontMetrics.height();
#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
    charWidth = fontMetrics.width('A');
#else
    charWidth = fontMetrics.horizontalAdvance('A');
#endif

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

bool HexWidget::moveCursor(int offset, bool select, OverflowMove overflowMove)
{
    BasicCursor addr(cursor.address);
    if (overflowMove == OverflowMove::Ignore) {
        if (addr.moveChecked(offset)) {
            if (addr.address > data->maxIndex()) {
                addr.address = data->maxIndex();
                addr.pastEnd = true;
            }
            setCursorAddr(addr, select);
            return true;
        }
        return false;
    } else {
        addr += offset;
        if (addr.address > data->maxIndex()) {
            addr.address = data->maxIndex();
        }
        setCursorAddr(addr, select);
        return true;
    }
}

void HexWidget::moveCursorKeepEditOffset(int byteOffset, bool select, OverflowMove overflowMove)
{
    int wordOffset = editWordPos;
    moveCursor(byteOffset, select, overflowMove);
    // preserve position within word when moving vertically in hex or oct modes
    if (!cursorOnAscii && !select && wordOffset > 0 && navigationMode == HexNavigationMode::AnyChar
        && editWordState > EditWordState::Read) {
        startEditWord();
        editWordPos = wordOffset;
    }
}

void HexWidget::setCursorAddr(BasicCursor addr, bool select)
{
    finishEditingWord();
    if (!select) {
        bool clearingSelection = !selection.isEmpty();
        selection.init(addr);
        if (clearingSelection)
            emit selectionChanged(getSelection());
    }
    emit positionChanged(addr.address);

    cursor.address = addr.address;
    if (!cursorOnAscii) {
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
    if (!(addressValue >= startAddress && addressValue <= lastVisibleAddr())) {
        /* Align start address */
        addressValue -= (addressValue % itemRowByteLen());

        /* FIXME: handling Page Up/Down */
        uint64_t rowAfterVisibleAddress = startAddress + bytesPerScreen();
        if (addressValue == rowAfterVisibleAddress && addressValue > startAddress) {
            // when pressing down add only one new row
            startAddress += itemRowByteLen();
        } else {
            startAddress = addressValue;
        }

        fetchData();

        if (startAddress > (data->maxIndex() - bytesPerScreen()) + 1) {
            startAddress = (data->maxIndex() - bytesPerScreen()) + 1;
        }
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

    if (editWordState > EditWordState::Read && !cursorOnAscii) {
        point.rx() += itemPrefixLen * charWidth;
    }

    cursor.screenPos.moveTopLeft(cursorOnAscii ? pointAscii : point);
    shadowCursor.screenPos.moveTopLeft(cursorOnAscii ? point : pointAscii);
}

void HexWidget::setCursorOnAscii(bool ascii)
{
    cursorOnAscii = ascii;
}

QColor HexWidget::itemColor(uint8_t byte)
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

QVariant HexWidget::readItem(int offset, QColor *color)
{
    quint8 byte;
    quint16 word;
    quint32 dword;
    quint64 qword;
    float float32;
    double float64;

    quint8 bytes[sizeof(uint64_t)];
    data->copy(bytes, startAddress + offset, static_cast<size_t>(itemByteLen));
    const bool signedItem = itemFormat == ItemFormatSignedDec;

    if (color) {
        *color = defColor;
    }

    switch (itemByteLen) {
    case 1:
        byte = bytes[0];
        if (color)
            *color = itemColor(byte);
        if (!signedItem)
            return QVariant(static_cast<quint64>(byte));
        return QVariant(static_cast<qint64>(static_cast<qint8>(byte)));
    case 2:
        if (itemBigEndian)
            word = fromBigEndian<quint16>(bytes);
        else
            word = fromLittleEndian<quint16>(bytes);

        if (!signedItem)
            return QVariant(static_cast<quint64>(word));
        return QVariant(static_cast<qint64>(static_cast<qint16>(word)));
    case 4:
        if (itemBigEndian)
            dword = fromBigEndian<quint32>(bytes);
        else
            dword = fromLittleEndian<quint32>(bytes);

        if (itemFormat == ItemFormatFloat) {
            memcpy(&float32, &dword, sizeof(float32));
            return QVariant(float32);
        }
        if (!signedItem)
            return QVariant(static_cast<quint64>(dword));
        return QVariant(static_cast<qint64>(static_cast<qint32>(dword)));
    case 8:
        if (itemBigEndian)
            qword = fromBigEndian<quint64>(bytes);
        else
            qword = fromLittleEndian<quint64>(bytes);
        if (itemFormat == ItemFormatFloat) {
            memcpy(&float64, &qword, sizeof(float64));
            return QVariant(float64);
        }
        if (!signedItem)
            return QVariant(qword);
        return QVariant(static_cast<qint64>(qword));
    }

    return QVariant();
}

QString HexWidget::renderItem(int offset, QColor *color)
{
    QString item;
    QVariant itemVal = readItem(offset, color);
    int itemLen = itemCharLen - itemPrefixLen; /* Reserve space for prefix */

    // FIXME: handle broken itemVal ( QVariant() )
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
        item = QString("%1").arg(itemVal.toDouble(), itemLen, 'g', itemByteLen == 4 ? 6 : 15);
        break;
    }

    return item;
}

QChar HexWidget::renderAscii(int offset, QColor *color)
{
    uchar byte;
    data->copy(&byte, startAddress + offset, sizeof(byte));
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
QString HexWidget::getFlagsAndComment(uint64_t address)
{
    QString flagNames = Core()->listFlagsAsStringAt(address);
    QString metaData = flagNames.isEmpty() ? "" : "Flags: " + flagNames.trimmed();

    QString comment = Core()->getCommentAt(address);
    if (!comment.isEmpty()) {
        if (!metaData.isEmpty()) {
            metaData.append("\n");
        }
        metaData.append("Comment: " + comment.trimmed());
    }

    return metaData;
}

bool HexWidget::canKeyboardEdit()
{
    return ioModesController.canWrite() && actionKeyboardEdit->isChecked();
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

bool HexWidget::parseWord(QString word, uint8_t *buf, size_t bufferSize) const
{
    bool parseOk = false;
    if (bufferSize < size_t(itemByteLen)) {
        return false;
    }
    if (itemFormat == ItemFormatFloat) {
        if (itemByteLen == 4) {
            float value = word.toFloat(&parseOk);
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
            double value = word.toDouble(&parseOk);
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

bool HexWidget::flushCurrentlyEditedWord()
{
    if (editWordState < EditWordState::WriteEdited) {
        return true;
    }
    uint8_t buf[16];
    if (parseWord(editWord, buf, sizeof(buf))) {
        data->write(buf, cursor.address, itemByteLen);
        return true;
    }
    editWordState = EditWordState::WriteNotEdited;
    return false;
}

bool HexWidget::finishEditingWord(bool force)
{
    if (editWordState == EditWordState::WriteEdited) {
        if (!flushCurrentlyEditedWord() && !force) {
            qWarning() << "Not a valid number in current format or size" << editWord;
            showWarningRect(itemRectangle(cursor.address - startAddress).adjusted(-1, -1, 1, 1));
            return false;
        }
    }
    editWord.clear();
    editWordPos = 0;
    editWordState = canKeyboardEdit() ? EditWordState::WriteNotStarted : EditWordState::Read;
    navigationMode = defaultNavigationMode();
    return true;
}

void HexWidget::cancelEditedWord()
{
    editWordPos = 0;
    editWordState = canKeyboardEdit() ? EditWordState::WriteNotStarted : EditWordState::Read;
    editWord.clear();
    navigationMode = defaultNavigationMode();
    updateCursorMeta();
    viewport()->update();
}

void HexWidget::maybeFlushCharEdit()
{
    if (editWordState < EditWordState::WriteEdited) {
        return;
    }
    if ((itemFormat == ItemFormatHex && earlyEditFlush >= EarlyEditFlush::EditNibble)
        || (isFixedWidth() && earlyEditFlush >= EarlyEditFlush::EditFixedWidthChar)) {
        flushCurrentlyEditedWord();
        if (!flushCurrentlyEditedWord()) {
            showWarningRect(itemRectangle(cursor.address - startAddress).adjusted(-1, -1, 1, 1));
        }
    }
    viewport()->update();
}

void HexWidget::startEditWord()
{
    if (!canKeyboardEdit()) {
        return;
    }
    if (editWordState >= EditWordState::WriteNotEdited) {
        return;
    }
    editWordPos = 0;
    editWordState = EditWordState::WriteNotEdited;
    navigationMode = defaultNavigationMode();
    editWord = renderItem(cursor.address - startAddress).trimmed();
    if (itemPrefixLen > 0) {
        editWord = editWord.mid(itemPrefixLen);
    }
    viewport()->update();
}

void HexWidget::fetchData()
{
    data.swap(oldData);
    data->fetch(startAddress, bytesPerScreen());
}

BasicCursor HexWidget::screenPosToAddr(const QPoint &point, bool middle, int *wordOffset) const
{
    QPointF pt = point - itemArea.topLeft();

    int relativeAddress = 0;
    int line = static_cast<int>(pt.y() / lineHeight);
    relativeAddress += line * itemRowByteLen();
    int column = static_cast<int>(pt.x() / columnExWidth());
    relativeAddress += column * itemGroupByteLen();
    pt.rx() -= column * columnExWidth();
    auto roundingOffset = middle ? itemWidth() / 2 : 0;
    int posInGroup = static_cast<int>((pt.x() + roundingOffset) / itemWidth());
    if (!middle) {
        posInGroup = std::min(posInGroup, itemGroupSize - 1);
    }
    relativeAddress += posInGroup * itemByteLen;
    pt.rx() -= posInGroup * itemWidth();
    BasicCursor result(startAddress);
    result += relativeAddress;

    if (!middle && wordOffset != nullptr) {
        int charPos = static_cast<int>((pt.x() / charWidth) + 0.5);
        charPos -= itemPrefixLen;
        charPos = std::max(0, charPos);
        *wordOffset = charPos;
    }
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
    return asciiArea.contains(point) ? asciiPosToAddr(point, middle)
                                     : screenPosToAddr(point, middle);
}

QRectF HexWidget::itemRectangle(int offset)
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

QRectF HexWidget::asciiRectangle(int offset)
{
    QPointF p;

    p.ry() = (offset / itemRowByteLen()) * lineHeight;
    offset %= itemRowByteLen();

    p.rx() = offset * charWidth;

    p += asciiArea.topLeft();

    return QRectF(p, QSizeF(charWidth, lineHeight));
}

RVA HexWidget::getLocationAddress()
{
    return !selection.isEmpty() ? selection.start() : cursor.address;
}

void HexWidget::hideWarningRect()
{
    warningRectVisible = false;
    viewport()->update();
}

void HexWidget::showWarningRect(QRectF rect)
{
    warningRect = rect;
    warningRectVisible = true;
    warningTimer.start(WARNING_TIME_MS);
    viewport()->update();
}
