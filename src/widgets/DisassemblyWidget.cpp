#include "DisassemblyWidget.h"
#include "menus/DisassemblyContextMenu.h"
#include "common/Configuration.h"
#include "common/DisassemblyPreview.h"
#include "common/Helpers.h"
#include "common/TempConfig.h"
#include "common/SelectionHighlight.h"
#include "common/BinaryTrees.h"
#include "core/MainWindow.h"
#include "shortcuts/ShortcutManager.h"

#include <QApplication>
#include <QScrollBar>
#include <QJsonArray>
#include <QJsonObject>
#include <QVBoxLayout>
#include <QRegularExpression>
#include <QtMath>
#include <QTextBlockUserData>
#include <QPainter>
#include <QPainterPath>
#include <QSplitter>

#include <algorithm>
#include <cmath>
#include <cstring>

DisassemblyWidget::DisassemblyWidget(MainWindow *main)
    : MemoryDockWidget(MemoryWidgetType::Disassembly, main),
      mCtxMenu(new DisassemblyContextMenu(this, main)),
      mDisasScrollArea(new DisassemblyScrollArea(this)),
      mDisasTextEdit(new DisassemblyTextEdit(this))
{
    setObjectName(main ? main->getUniqueObjectName(getWidgetType()) : getWidgetType());
    updateWindowTitle();

    topOffset = bottomOffset = RVA_INVALID;
    cursorLineOffset = 0;
    cursorCharOffset = 0;
    seekFromCursor = false;

    // Instantiate the window layout
    auto *splitter = new QSplitter;

    // Setup the left frame that contains breakpoints and jumps
    leftPanel = new DisassemblyLeftPanel(this);
    splitter->addWidget(leftPanel);

    // Setup the disassembly content
    auto *layout = new QHBoxLayout;
    layout->addWidget(mDisasTextEdit);
    layout->setContentsMargins(0, 0, 0, 0);
    mDisasScrollArea->viewport()->setLayout(layout);
    splitter->addWidget(mDisasScrollArea);
    connect(mDisasScrollArea->verticalScrollBar(), &QScrollBar::valueChanged, this,
            [this](int) { refreshDisasm(mDisasScrollArea->currentVScrollAddr()); });
    // Use stylesheet instead of QWidget::setFrameShape(QFrame::NoShape) to avoid
    // issues with dark and light interface themes
    mDisasScrollArea->setStyleSheet("QAbstractScrollArea { border: 0px transparent black; }");
    mDisasTextEdit->setStyleSheet("QPlainTextEdit { border: 0px transparent black; }");
    mDisasTextEdit->setFocusProxy(this);
    mDisasTextEdit->setFocusPolicy(Qt::ClickFocus);
    mDisasScrollArea->setFocusProxy(this);
    mDisasScrollArea->setFocusPolicy(Qt::ClickFocus);

    setFocusPolicy(Qt::ClickFocus);

    // Behave like all widgets: highlight on focus and hover
    connect(qApp, &QApplication::focusChanged, this, [this](QWidget *, QWidget *now) {
        QColor borderColor = this == now ? palette().color(QPalette::Highlight)
                                         : palette().color(QPalette::WindowText).darker();
        widget()->setStyleSheet(QString("QSplitter { border: %1px solid %2 } \n"
                                        "QSplitter:hover { border: %1px solid %3 } \n")
                                        .arg(devicePixelRatio())
                                        .arg(borderColor.name())
                                        .arg(palette().color(QPalette::Highlight).name()));
    });

    splitter->setFrameShape(QFrame::Box);
    // Set current widget to the splitted layout we just created
    setWidget(splitter);

    // Resize properly
    QList<int> sizes;
    sizes.append(3);
    sizes.append(1);
    splitter->setSizes(sizes);

    setAllowedAreas(Qt::AllDockWidgetAreas);

    setupFonts();
    setupColors();

    disasmRefresh = createReplacingRefreshDeferrer<RVA>(
            false, [this](const RVA *offset) { refreshDisasm(offset ? *offset : RVA_INVALID); });

    maxLines = 0;
    updateMaxLines();

    mDisasTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mDisasTextEdit->setFont(Config()->getFont());
    mDisasTextEdit->setReadOnly(true);
    mDisasTextEdit->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    // wrapping breaks readCurrentDisassemblyOffset() at the moment :-(
    mDisasTextEdit->setWordWrapMode(QTextOption::NoWrap);

    // Increase asm text edit margin
    QTextDocument *asm_docu = mDisasTextEdit->document();
    asm_docu->setDocumentMargin(10);

    // Event filter to intercept double clicks in the textbox
    // and showing tooltips when hovering above those offsets
    mDisasTextEdit->viewport()->installEventFilter(this);

    // Set Disas context menu
    mDisasTextEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(mDisasTextEdit, &QWidget::customContextMenuRequested, this,
            &DisassemblyWidget::showDisasContextMenu);

    connect(mDisasScrollArea, &DisassemblyScrollArea::scrollLines, this,
            &DisassemblyWidget::scrollInstructions);
    connect(mDisasScrollArea, &DisassemblyScrollArea::disassemblyResized, this,
            &DisassemblyWidget::updateMaxLines);

    connectCursorPositionChanged(false);
    connect(mDisasTextEdit->verticalScrollBar(), &QScrollBar::valueChanged, this, [=](int value) {
        if (value != 0) {
            mDisasTextEdit->verticalScrollBar()->setValue(0);
        }
    });

    connect(Core(), &CutterCore::commentsChanged, this, [this]() { refreshDisasm(); });
    connect(Core(), SIGNAL(flagsChanged()), this, SLOT(refreshDisasm()));
    connect(Core(), SIGNAL(globalVarsChanged()), this, SLOT(refreshDisasm()));
    connect(Core(), SIGNAL(functionsChanged()), this, SLOT(refreshDisasm()));
    connect(Core(), &CutterCore::functionRenamed, this, [this]() { refreshDisasm(); });
    connect(Core(), SIGNAL(varsChanged()), this, SLOT(refreshDisasm()));
    connect(Core(), SIGNAL(asmOptionsChanged()), this, SLOT(refreshDisasm()));
    connect(Core(), &CutterCore::instructionChanged, this, &DisassemblyWidget::instructionChanged);
    connect(Core(), &CutterCore::breakpointsChanged, this, &DisassemblyWidget::refreshIfInRange);
    connect(Core(), SIGNAL(refreshCodeViews()), this, SLOT(refreshDisasm()));

    connect(Config(), &Configuration::fontsUpdated, this, &DisassemblyWidget::fontsUpdatedSlot);
    connect(Config(), &Configuration::colorsUpdated, this, &DisassemblyWidget::colorsUpdatedSlot);

    connect(Core(), &CutterCore::refreshAll, this,
            [this]() { refreshDisasm(seekable->getOffset()); });
    refreshDisasm(seekable->getOffset());

    connect(mCtxMenu, &DisassemblyContextMenu::copy, mDisasTextEdit, &QPlainTextEdit::copy);

    mCtxMenu->addSeparator();
    mCtxMenu->addAction(&syncAction);
    connect(seekable, &CutterSeekable::seekableSeekChanged, this,
            &DisassemblyWidget::on_seekChanged);

    addActions(mCtxMenu->actions());

#define ADD_ACTION(id, ctx, slot)                                                                  \
    {                                                                                              \
        QAction *a = Shortcuts()->makeAction(id, this);                                            \
        a->setShortcutContext(ctx);                                                                \
        addAction(a);                                                                              \
        connect(a, &QAction::triggered, this, (slot));                                             \
    }

    // Space to switch to graph
    ADD_ACTION("Disassembly.switchToGraph", Qt::WidgetWithChildrenShortcut,
               [this] { mainWindow->showMemoryWidget(MemoryWidgetType::Graph); })

    ADD_ACTION("General.seekPrev", Qt::WidgetWithChildrenShortcut, &DisassemblyWidget::seekPrev)

    ADD_ACTION("Disassembly.moveDown", Qt::WidgetWithChildrenShortcut,
               [this]() { moveCursorRelative(false, false); })
    ADD_ACTION("Disassembly.moveUp", Qt::WidgetWithChildrenShortcut,
               [this]() { moveCursorRelative(true, false); })
    ADD_ACTION("Disassembly.pageDown", Qt::WidgetWithChildrenShortcut,
               [this]() { moveCursorRelative(false, true); })
    ADD_ACTION("Disassembly.pageUp", Qt::WidgetWithChildrenShortcut,
               [this]() { moveCursorRelative(true, true); })
#undef ADD_ACTION
}

void DisassemblyWidget::setPreviewMode(bool previewMode)
{
    mDisasTextEdit->setContextMenuPolicy(previewMode ? Qt::NoContextMenu : Qt::CustomContextMenu);
    mCtxMenu->setEnabled(!previewMode);
    for (auto action : mCtxMenu->actions()) {
        action->setEnabled(!previewMode);
    }
    for (auto action : actions()) {
        if (action->shortcut() == Qt::Key_Space || action->shortcut() == Qt::Key_Escape) {
            action->setEnabled(!previewMode);
        }
    }
    if (previewMode) {
        seekable->setSynchronization(false);
    }
}

QWidget *DisassemblyWidget::getTextWidget()
{
    return mDisasTextEdit;
}

QString DisassemblyWidget::getWidgetType()
{
    return "Disassembly";
}

QFontMetricsF DisassemblyWidget::getFontMetrics()
{
    return QFontMetricsF(mDisasTextEdit->font());
}

QList<DisassemblyLine> DisassemblyWidget::getLines()
{
    return lines;
}

void DisassemblyWidget::refreshIfInRange(RVA offset)
{
    if (offset >= topOffset && offset <= bottomOffset) {
        refreshDisasm();
    }
}

void DisassemblyWidget::instructionChanged(RVA offset)
{
    leftPanel->clearArrowFrom(offset);
    refreshDisasm();
}

void DisassemblyWidget::refreshDisasm(RVA offset)
{
    if (!disasmRefresh->attemptRefresh(offset == RVA_INVALID ? nullptr : new RVA(offset))) {
        return;
    }

    if (offset != RVA_INVALID) {
        topOffset = offset;
    }

    if (topOffset == RVA_INVALID) {
        return;
    }

    if (maxLines <= 0) {
        connectCursorPositionChanged(true);
        mDisasTextEdit->clear();
        connectCursorPositionChanged(false);
        return;
    }

    breakpoints = Core()->getBreakpointsAddresses();
    int horizontalScrollValue = mDisasTextEdit->horizontalScrollBar()->value();
    mDisasTextEdit->setLockScroll(true); // avoid flicker

    // Retrieve disassembly lines
    {
        TempConfig tempConfig;
        tempConfig.set("scr.color", COLOR_MODE_16M).set("asm.lines", false);
        lines = Core()->disassembleLines(topOffset, maxLines);
    }

    connectCursorPositionChanged(true);

    mDisasTextEdit->document()->clear();
    QTextCursor cursor(mDisasTextEdit->document());
    QTextBlockFormat regular = cursor.blockFormat();
    for (const DisassemblyLine &line : lines) {
        if (line.offset < topOffset) { // overflow
            break;
        }
        cursor.insertHtml(line.text);
        if (Core()->isBreakpoint(breakpoints, line.offset)) {
            QTextBlockFormat f;
            f.setBackground(ConfigColor("gui.breakpoint_background"));
            cursor.setBlockFormat(f);
        }
        auto a = new DisassemblyTextBlockUserData(line);
        cursor.block().setUserData(a);
        cursor.insertBlock();
        cursor.setBlockFormat(regular);
    }

    if (!lines.isEmpty()) {
        bottomOffset = lines[qMin(lines.size(), maxLines) - 1].offset;
        if (bottomOffset < topOffset) {
            bottomOffset = RVA_MAX;
        }
    } else {
        bottomOffset = topOffset;
    }

    connectCursorPositionChanged(false);

    updateCursorPosition();

    // remove additional lines
    QTextCursor tc = mDisasTextEdit->textCursor();
    tc.movePosition(QTextCursor::Start);
    tc.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, maxLines - 1);
    tc.movePosition(QTextCursor::EndOfLine);
    tc.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    tc.removeSelectedText();

    mDisasTextEdit->setLockScroll(false);
    mDisasTextEdit->horizontalScrollBar()->setValue(horizontalScrollValue);
    mDisasScrollArea->setVScrollPos(topOffset);

    // Refresh the left panel (trigger paintEvent)
    leftPanel->update();
}

void DisassemblyWidget::scrollInstructions(int count)
{
    if (count == 0) {
        return;
    }

    RVA offset;
    if (count > 0) {
        offset = Core()->nextOpAddr(topOffset, count);
        if (offset < topOffset) {
            offset = RVA_MAX;
        }
    } else {
        offset = Core()->prevOpAddr(topOffset, -count);
        if (offset > topOffset) {
            offset = 0;
        }
    }

    refreshDisasm(offset);
    topOffsetHistory[topOffsetHistoryPos] = offset;
}

bool DisassemblyWidget::updateMaxLines()
{
    int currentMaxLines = qhelpers::getMaxFullyDisplayedLines(mDisasTextEdit);

    if (currentMaxLines != maxLines) {
        maxLines = currentMaxLines;
        refreshDisasm();
        return true;
    }

    return false;
}

void DisassemblyWidget::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    QColor highlightColor = ConfigColor("lineHighlight");

    // Highlight the current word
    QTextCursor cursor = mDisasTextEdit->textCursor();
    auto clickedCharPos = cursor.positionInBlock();
    // Select the line (BlockUnderCursor matches a line with current implementation)
    cursor.select(QTextCursor::BlockUnderCursor);
    // Remove any non-breakable space from the current line
    QString searchString = cursor.selectedText().replace("\xc2\xa0", " ");
    // Cut the line in "tokens" that can be highlighted
    static const QRegularExpression tokenRegExp(R"(\b(?<!\.)([^\s]+)\b(?!\.))");
    QRegularExpressionMatchIterator i = tokenRegExp.globalMatch(searchString);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        // Current token is under our cursor, select this one
        if (match.capturedStart() <= clickedCharPos && match.capturedEnd() > clickedCharPos) {
            curHighlightedWord = match.captured();
            break;
        }
    }

    // Highlight the current line
    QTextEdit::ExtraSelection highlightSelection;
    highlightSelection.cursor = cursor;
    highlightSelection.cursor.movePosition(QTextCursor::Start);
    while (true) {
        RVA lineOffset = DisassemblyPreview::readDisassemblyOffset(highlightSelection.cursor);
        if (lineOffset == seekable->getOffset()) {
            highlightSelection.format.setBackground(highlightColor);
            highlightSelection.format.setProperty(QTextFormat::FullWidthSelection, true);
            highlightSelection.cursor.clearSelection();
            extraSelections.append(highlightSelection);
        } else if (lineOffset != RVA_INVALID && lineOffset > seekable->getOffset()) {
            break;
        }
        highlightSelection.cursor.movePosition(QTextCursor::EndOfLine);
        if (highlightSelection.cursor.atEnd()) {
            break;
        }

        highlightSelection.cursor.movePosition(QTextCursor::Down);
    }

    // Highlight all the words in the document same as the current one
    extraSelections.append(createSameWordsSelections(mDisasTextEdit, curHighlightedWord));

    mDisasTextEdit->setExtraSelections(extraSelections);
}

void DisassemblyWidget::highlightPCLine()
{
    RVA PCAddr = Core()->getProgramCounterValue();

    QColor highlightPCColor = ConfigColor("highlightPC");

    QList<QTextEdit::ExtraSelection> pcSelections;
    QTextEdit::ExtraSelection highlightSelection;
    highlightSelection.cursor = mDisasTextEdit->textCursor();
    highlightSelection.cursor.movePosition(QTextCursor::Start);
    if (PCAddr != RVA_INVALID) {
        while (true) {
            RVA lineOffset = DisassemblyPreview::readDisassemblyOffset(highlightSelection.cursor);
            if (lineOffset == PCAddr) {
                highlightSelection.format.setBackground(highlightPCColor);
                highlightSelection.format.setProperty(QTextFormat::FullWidthSelection, true);
                highlightSelection.cursor.clearSelection();
                pcSelections.append(highlightSelection);
            } else if (lineOffset != RVA_INVALID && lineOffset > PCAddr) {
                break;
            }
            highlightSelection.cursor.movePosition(QTextCursor::EndOfLine);
            if (highlightSelection.cursor.atEnd()) {
                break;
            }

            highlightSelection.cursor.movePosition(QTextCursor::Down);
        }
    }

    // Don't override any extraSelections already set
    QList<QTextEdit::ExtraSelection> currentSelections = mDisasTextEdit->extraSelections();
    currentSelections.append(pcSelections);

    mDisasTextEdit->setExtraSelections(currentSelections);
}

void DisassemblyWidget::showDisasContextMenu(const QPoint &pt)
{
    mCtxMenu->exec(mDisasTextEdit->mapToGlobal(pt));
}

RVA DisassemblyWidget::readCurrentDisassemblyOffset()
{
    QTextCursor tc = mDisasTextEdit->textCursor();
    return DisassemblyPreview::readDisassemblyOffset(tc);
}

void DisassemblyWidget::updateCursorPosition()
{
    RVA offset = seekable->getOffset();

    // already fine where it is?
    RVA currentLineOffset = readCurrentDisassemblyOffset();
    if (currentLineOffset == offset) {
        return;
    }

    connectCursorPositionChanged(true);

    if (offset < topOffset || (offset > bottomOffset && bottomOffset != RVA_INVALID)) {
        mDisasTextEdit->moveCursor(QTextCursor::Start);
        mDisasTextEdit->setExtraSelections(
                createSameWordsSelections(mDisasTextEdit, curHighlightedWord));
    } else {
        RVA currentCursorOffset = readCurrentDisassemblyOffset();
        QTextCursor originalCursor = mDisasTextEdit->textCursor();

        QTextCursor cursor = originalCursor;
        cursor.movePosition(QTextCursor::Start);

        while (true) {
            RVA lineOffset = DisassemblyPreview::readDisassemblyOffset(cursor);
            if (lineOffset == offset) {
                if (cursorLineOffset > 0) {
                    cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor,
                                        cursorLineOffset);
                }
                if (cursorCharOffset > 0) {
                    cursor.movePosition(QTextCursor::StartOfLine);
                    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor,
                                        cursorCharOffset);
                }

                mDisasTextEdit->setTextCursor(cursor);
                highlightCurrentLine();
                break;
            } else if (lineOffset != RVA_INVALID && lineOffset > offset) {
                mDisasTextEdit->moveCursor(QTextCursor::Start);
                mDisasTextEdit->setExtraSelections({});
                break;
            }

            cursor.movePosition(QTextCursor::EndOfLine);
            if (cursor.atEnd()) {
                break;
            }

            cursor.movePosition(QTextCursor::Down);
        }

        // this is true if a seek came from the user clicking on a line.
        // then the cursor should be restored 1:1 to retain selection and cursor position.
        if (currentCursorOffset == offset) {
            mDisasTextEdit->setTextCursor(originalCursor);
        }
    }

    highlightPCLine();

    connectCursorPositionChanged(false);
}

void DisassemblyWidget::connectCursorPositionChanged(bool disconnect)
{
    if (disconnect) {
        QObject::disconnect(mDisasTextEdit, &QPlainTextEdit::cursorPositionChanged, this,
                            &DisassemblyWidget::cursorPositionChanged);
    } else {
        connect(mDisasTextEdit, &QPlainTextEdit::cursorPositionChanged, this,
                &DisassemblyWidget::cursorPositionChanged);
    }
}

void DisassemblyWidget::cursorPositionChanged()
{
    RVA offset = readCurrentDisassemblyOffset();

    cursorLineOffset = 0;
    QTextCursor c = mDisasTextEdit->textCursor();
    cursorCharOffset = c.positionInBlock();
    while (c.blockNumber() > 0) {
        c.movePosition(QTextCursor::PreviousBlock);
        if (DisassemblyPreview::readDisassemblyOffset(c) != offset) {
            break;
        }
        cursorLineOffset++;
    }

    seekFromCursor = true;
    seekable->seek(offset);
    seekFromCursor = false;
    highlightCurrentLine();
    highlightPCLine();
    mCtxMenu->setCanCopy(mDisasTextEdit->textCursor().hasSelection());
    if (mDisasTextEdit->textCursor().hasSelection()) {
        // A word is selected so use it
        mCtxMenu->setCurHighlightedWord(mDisasTextEdit->textCursor().selectedText());
    } else {
        // No word is selected so use the word under the cursor
        mCtxMenu->setCurHighlightedWord(curHighlightedWord);
    }
    leftPanel->update();
}

void DisassemblyWidget::moveCursorRelative(bool up, bool page)
{
    if (page) {
        RVA offset;
        if (!up) {
            offset = Core()->nextOpAddr(bottomOffset, 1);
            if (offset < bottomOffset) {
                offset = RVA_MAX;
            }
        } else {
            offset = Core()->prevOpAddr(topOffset, maxLines);
            if (offset > topOffset) {
                offset = 0;
            } else {
                // disassembly from calculated offset may have more than maxLines lines
                // move some instructions down if necessary.

                auto lines = Core()->disassembleLines(offset, maxLines).toVector();
                int oldTopLine;
                for (oldTopLine = lines.length(); oldTopLine > 0; oldTopLine--) {
                    if (lines[oldTopLine - 1].offset < topOffset) {
                        break;
                    }
                }

                int overflowLines = oldTopLine - maxLines;
                if (overflowLines > 0) {
                    while (lines[overflowLines - 1].offset == lines[overflowLines].offset
                           && overflowLines < lines.length() - 1) {
                        overflowLines++;
                    }
                    offset = lines[overflowLines].offset;
                }
            }
        }
        refreshDisasm(offset);
    } else { // normal arrow keys
        int blockCount = mDisasTextEdit->blockCount();
        if (blockCount < 1) {
            return;
        }

        int blockNumber = mDisasTextEdit->textCursor().blockNumber();

        if (blockNumber == blockCount - 1 && !up) {
            scrollInstructions(1);
        } else if (blockNumber == 0 && up) {
            scrollInstructions(-1);
        }

        mDisasTextEdit->moveCursor(up ? QTextCursor::Up : QTextCursor::Down);

        // handle cases where top instruction offsets change
        RVA offset = readCurrentDisassemblyOffset();
        if (offset != seekable->getOffset()) {
            seekable->seek(offset);
            highlightCurrentLine();
            highlightPCLine();
        }
    }
}

void DisassemblyWidget::jumpToOffsetUnderCursor(const QTextCursor &cursor)
{
    RVA offset = DisassemblyPreview::readDisassemblyOffset(cursor);
    seekable->seekToReference(offset);
}

bool DisassemblyWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonDblClick
        && (obj == mDisasTextEdit || obj == mDisasTextEdit->viewport())) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

        if (mouseEvent->button() == Qt::LeftButton) {
            const QTextCursor &cursor = mDisasTextEdit->cursorForPosition(mouseEvent->pos());
            jumpToOffsetUnderCursor(cursor);

            return true;
        }
    } else if ((Config()->getPreviewValue() || Config()->getShowVarTooltips())
               && event->type() == QEvent::ToolTip && obj == mDisasTextEdit->viewport()) {
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
        auto cursorForWord = mDisasTextEdit->cursorForPosition(helpEvent->pos());
        cursorForWord.select(QTextCursor::WordUnderCursor);

        RVA offsetFrom = DisassemblyPreview::readDisassemblyOffset(cursorForWord);

        if (Config()->getPreviewValue()
            && DisassemblyPreview::showDisasPreview(this, helpEvent->globalPos(), offsetFrom)) {
            return true;
        }
        if (Config()->getShowVarTooltips()
            && DisassemblyPreview::showDebugValueTooltip(
                    this, helpEvent->globalPos(), cursorForWord.selectedText(), offsetFrom)) {
            return true;
        }
    }

    return MemoryDockWidget::eventFilter(obj, event);
}

void DisassemblyWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return) {
        const QTextCursor cursor = mDisasTextEdit->textCursor();
        jumpToOffsetUnderCursor(cursor);
    }

    MemoryDockWidget::keyPressEvent(event);
}

QString DisassemblyWidget::getWindowTitle() const
{
    return tr("Disassembly");
}

void DisassemblyWidget::on_seekChanged(RVA offset, CutterCore::SeekHistoryType type)
{
    if (type == CutterCore::SeekHistoryType::New) {
        // Erase previous history past this point.
        if (topOffsetHistory.size() > topOffsetHistoryPos + 1) {
            topOffsetHistory.erase(topOffsetHistory.begin() + topOffsetHistoryPos + 1,
                                   topOffsetHistory.end());
        }
        topOffsetHistory.push_back(offset);
        topOffsetHistoryPos = topOffsetHistory.size() - 1;
    } else if (type == CutterCore::SeekHistoryType::Undo) {
        --topOffsetHistoryPos;
    } else if (type == CutterCore::SeekHistoryType::Redo) {
        ++topOffsetHistoryPos;
    }
    if (!seekFromCursor) {
        cursorLineOffset = 0;
        cursorCharOffset = 0;
    }

    if (topOffset != RVA_INVALID && offset >= topOffset && offset <= bottomOffset
        && type == CutterCore::SeekHistoryType::New) {
        // if the line with the seek offset is currently visible, just move the cursor there
        updateCursorPosition();
        topOffsetHistory[topOffsetHistoryPos] = topOffset;
    } else {
        // otherwise scroll there
        refreshDisasm(topOffsetHistory[topOffsetHistoryPos]);
    }
    mCtxMenu->setOffset(offset);
}

void DisassemblyWidget::fontsUpdatedSlot()
{
    setupFonts();

    if (!updateMaxLines()) { // updateMaxLines() returns true if it already refreshed.
        refreshDisasm();
    }
}

void DisassemblyWidget::colorsUpdatedSlot()
{
    setupColors();
    refreshDisasm();
}

void DisassemblyWidget::setupFonts()
{
    mDisasTextEdit->setFont(Config()->getFont());
}

void DisassemblyWidget::setupColors()
{
    mDisasTextEdit->setStyleSheet(QString("QPlainTextEdit { background-color: %1; color: %2; }")
                                          .arg(ConfigColor("gui.background").name())
                                          .arg(ConfigColor("btext").name()));

    // Read and set a stylesheet for the QToolTip too
    setStyleSheet(DisassemblyPreview::getToolTipStyleSheet());
}

DisassemblyScrollArea::DisassemblyScrollArea(QWidget *parent) : QAbstractScrollArea(parent)
{
    beginOffset = RVA_INVALID;
    endOffset = RVA_INVALID;
    accumScrollWheelDeltaY = 0;
    verticalScrollBar()->setPageStep(40);
    connect(verticalScrollBar(), &QScrollBar::actionTriggered, this, [this](int action) {
        QScrollBar *vScrollBar = verticalScrollBar();
        int val = vScrollBar->value();
        switch (action) {
        case QAbstractSlider::SliderSingleStepAdd:
            // Due to the way the QScrollBar::actionTriggered signal works,
            // setting the slider pos to its current value here
            // prevents it from moving, allowing us to basically
            // override the scroll bar buttons' behavior
            // See https://doc.qt.io/qt-6/qabstractslider.html#actionTriggered
            // for more info.
            vScrollBar->setSliderPosition(val);
            if (val != vScrollBar->maximum()) {
                emit scrollLines(1);
            }
            return;
        case QAbstractSlider::SliderSingleStepSub:
            // Same as above
            vScrollBar->setSliderPosition(val);
            if (val != vScrollBar->minimum()) {
                emit scrollLines(-1);
            }
            return;
        default:
            break;
        }
    });
    refreshVScrollbarRange();
    connect(Core(), &CutterCore::refreshAll, this, &DisassemblyScrollArea::refreshVScrollbarRange);
}

RVA DisassemblyScrollArea::binSize()
{
    return endOffset - beginOffset;
}

RVA DisassemblyScrollArea::currentVScrollAddr()
{
    int maximum = verticalScrollBar()->maximum();
    if (!maximum || !binSize()) {
        return beginOffset;
    }
    // Fallback formula for large files
    if ((RVA_MAX / maximum) < binSize()) {
        return verticalScrollBar()->value() * (binSize() / maximum)
                + std::min<RVA>(verticalScrollBar()->value(), binSize() % maximum) + beginOffset;
    }
    return (verticalScrollBar()->value() * binSize()) / maximum + beginOffset;
}

void DisassemblyScrollArea::setVScrollPos(RVA address)
{
    const QSignalBlocker blocker(verticalScrollBar());
    int maximum = verticalScrollBar()->maximum();
    if (!maximum || !binSize()) {
        setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
        return;
    }
    int scrollBarPos = 0;
    if (address < beginOffset) {
        verticalScrollBar()->setValue(scrollBarPos);
        return;
    }
    if (address > endOffset) {
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
        return;
    }
    auto offset = address - beginOffset;
    if ((RVA_MAX / maximum) < binSize()) {
        // Fallback formula for large files
        uint64_t smallBox = binSize() / maximum;
        uint64_t extra = binSize() % maximum;
        auto bigBoxRange = (smallBox + 1) * extra;
        if (offset < bigBoxRange) {
            scrollBarPos = offset / (smallBox + 1);
        } else {
            scrollBarPos = extra + (offset - bigBoxRange) / smallBox;
        }
    } else {
        scrollBarPos = (maximum * offset) / binSize();
    }
    if (address != beginOffset && scrollBarPos == 0) {
        scrollBarPos = 1;
    }
    verticalScrollBar()->setValue(scrollBarPos);
}

void DisassemblyScrollArea::refreshVScrollbarRange()
{
    beginOffset = RVA_MAX;
    endOffset = 0;
    if (!Core()->currentlyEmulating && Core()->currentlyDebugging) {
        QString currentlyOpenFile = Core()->getConfig("file.path");
        QList<MemoryMapDescription> memoryMaps = Core()->getMemoryMap();
        for (const MemoryMapDescription &map : memoryMaps) {
            if (map.fileName == currentlyOpenFile) {
                if (map.addrStart < beginOffset) {
                    beginOffset = map.addrStart;
                }
                if (map.addrEnd > endOffset) {
                    endOffset = map.addrEnd;
                }
            }
        }
    } else {
        RzCoreLocked core(Core());
        RzPVector *mapsPtr = rz_io_maps(core->io);
        if (!mapsPtr) {
            setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
            return;
        }
        CutterPVector<RzIOMap> maps { mapsPtr };
        for (const RzIOMap *const map : maps) {
            // Skip the ESIL memory stack region
            if (Core()->currentlyEmulating && std::strncmp(rz_str_get(map->name), "mem.", 4) == 0) {
                continue;
            }
            ut64 b = rz_itv_begin(map->itv);
            ut64 e = rz_itv_end(map->itv);
            if (b < beginOffset) {
                beginOffset = b;
            }
            if (e > endOffset) {
                endOffset = e;
            }
        }
    }
    if (endOffset) {
        --endOffset;
    }
    if (endOffset == 0) {
        beginOffset = 0;
    }
    verticalScrollBar()->setMinimum(0);
    // Increasing this value increases scroll bar accuracy for small files but
    // decreases it for large files
    // Sufficiently below 2^32 to avoid causing problems in calculations done by QScrollbar,
    // otherwise as high as possible to maximize range in which address map 1:1 to scrollbar pos.
    const int rangeMax = 512 * 1024 * 1024;
    if (binSize() > rangeMax) {
        verticalScrollBar()->setMaximum(rangeMax);
    } else {
        verticalScrollBar()->setMaximum(binSize());
    }
    if (binSize()) {
        setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOn);
    } else {
        setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    }
}

bool DisassemblyScrollArea::viewportEvent(QEvent *event)
{
    if (event->type() == QEvent::Resize) {
        emit disassemblyResized();
    }

    return QAbstractScrollArea::viewportEvent(event);
}

void DisassemblyScrollArea::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().isNull() || !event->angleDelta().y()) {
        QAbstractScrollArea::wheelEvent(event);
        return;
    }
    accumScrollWheelDeltaY += event->angleDelta().y();
    // Delta is reported in 1/8 of a degree
    // eg. 120 units * 1/8 = 15 degrees
    // Typical scroll speed is 1 line per 5 degrees
    const int lineDelta = 5 * 8;
    if (accumScrollWheelDeltaY >= lineDelta || accumScrollWheelDeltaY <= -lineDelta) {
        int lineCount = accumScrollWheelDeltaY / lineDelta;
        accumScrollWheelDeltaY -= lineDelta * lineCount;
        emit scrollLines(-lineCount);
    }
}

qreal DisassemblyTextEdit::textOffset() const
{
    return (blockBoundingGeometry(document()->begin()).topLeft() + contentOffset()).y();
}

bool DisassemblyTextEdit::viewportEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::Type::Wheel:
        return false;
    default:
        return QAbstractScrollArea::viewportEvent(event);
    }
}

void DisassemblyTextEdit::scrollContentsBy(int dx, int dy)
{
    if (!lockScroll) {
        QPlainTextEdit::scrollContentsBy(dx, dy);
    }
}

void DisassemblyTextEdit::keyPressEvent(QKeyEvent *event)
{
    Q_UNUSED(event)
    // QPlainTextEdit::keyPressEvent(event);
}

void DisassemblyTextEdit::mousePressEvent(QMouseEvent *event)
{
    QPlainTextEdit::mousePressEvent(event);

    if (event->button() == Qt::RightButton && !textCursor().hasSelection()) {
        setTextCursor(cursorForPosition(event->pos()));
    }
}

void DisassemblyWidget::seekPrev()
{
    Core()->seekPrev();
}

/*********************
 * Left panel
 *********************/

DisassemblyLeftPanel::DisassemblyLeftPanel(DisassemblyWidget *disas)
{
    this->disas = disas;
    arrows.reserve((arrowsSize * 3) / 2);
}

void DisassemblyLeftPanel::wheelEvent(QWheelEvent *event)
{
    int count = -(event->angleDelta() / 15).y();
    count -= (count > 0 ? 5 : -5);

    this->disas->scrollInstructions(count);
}

void DisassemblyLeftPanel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    constexpr int penSizePix = 1;
    constexpr int distanceBetweenLines = 10;
    constexpr int arrowWidth = 5;
    int rightOffset = size().rwidth();
    auto tEdit = qobject_cast<DisassemblyTextEdit *>(disas->getTextWidget());
    int lineHeight = qCeil(disas->getFontMetrics().lineSpacing());
    QColor arrowColorDown = ConfigColor("flow");
    QColor arrowColorUp = ConfigColor("cflow");
    QPainter p(this);
    QPen penDown(arrowColorDown, penSizePix, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin);
    QPen penUp(arrowColorUp, penSizePix, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin);
    // Fill background
    p.fillRect(event->rect(), Config()->getColor("gui.background").darker(115));

    QList<DisassemblyLine> lines = disas->getLines();
    if (lines.size() == 0) {
        // No line to print, abort early
        return;
    }

    std::vector<int> lineY(tEdit->document()->lineCount());
    QTextCursor cursor(tEdit->document());
    for (auto &line : lineY) {
        auto rect = tEdit->cursorRect(cursor);
        line = rect.top();
        cursor.movePosition(QTextCursor::Down);
    }

    using LineInfo = std::pair<RVA, int>;
    std::vector<LineInfo> lineOffsets;
    lineOffsets.reserve(lines.size() + arrows.size());

    RVA minViewOffset = 0, maxViewOffset = 0;
    minViewOffset = maxViewOffset = lines[0].offset;

    for (int i = 0; i < lines.size(); i++) {
        lineOffsets.emplace_back(lines[i].offset, i);
        minViewOffset = std::min(minViewOffset, lines[i].offset);
        maxViewOffset = std::max(maxViewOffset, lines[i].offset);
        if (lines[i].arrow != RVA_INVALID) {
            Arrow a { lines[i].offset, lines[i].arrow };
            bool contains = std::find_if(std::begin(arrows), std::end(arrows),
                                         [&](const Arrow &it) {
                                             return it.min == a.min && it.max == a.max;
                                         })
                    != std::end(arrows);
            if (!contains) {
                arrows.emplace_back(lines[i].offset, lines[i].arrow);
            }
        }
    }

    auto addOffsetOutsideScreen = [&](RVA offset) {
        if (offset < minViewOffset || offset > maxViewOffset) {
            lineOffsets.emplace_back(offset, -1);
        }
    };

    // Assign sequential numbers to offsets outside screen while preserving their relative order.
    // Preserving relative order helps reducing reordering while scrolling. Using sequential numbers
    // allows using data structures designed for dense ranges.
    for (auto &arrow : arrows) {
        addOffsetOutsideScreen(arrow.min);
        addOffsetOutsideScreen(arrow.max);
    }
    std::sort(lineOffsets.begin(), lineOffsets.end());
    lineOffsets.erase(std::unique(lineOffsets.begin(), lineOffsets.end()), lineOffsets.end());
    size_t firstVisibleLine = std::find_if(lineOffsets.begin(), lineOffsets.end(),
                                           [](const LineInfo &line) { return line.second == 0; })
            - lineOffsets.begin();
    for (int i = int(firstVisibleLine) - 1; i >= 0; i--) {
        // -1 to ensure end of arrrow is drawn outside screen
        lineOffsets[i].second = i - firstVisibleLine - 1;
    }
    size_t firstLineAfter =
            std::find_if(lineOffsets.begin(), lineOffsets.end(),
                         [&](const LineInfo &line) { return line.first > maxViewOffset; })
            - lineOffsets.begin();
    for (size_t i = firstLineAfter; i < lineOffsets.size(); i++) {
        lineOffsets[i].second = lines.size() + (i - firstLineAfter)
                + 1; // +1 to ensure end of arrrow is drawn outside screen
    }

    auto offsetToLine = [&](RVA offset) -> int {
        // binary search because linesPixPosition is sorted by offset
        if (lineOffsets.empty()) {
            return 0;
        }
        if (offset < lineOffsets[0].first) {
            return lineOffsets[0].second - 1;
        }
        auto res = lower_bound(std::begin(lineOffsets), std::end(lineOffsets), offset,
                               [](const LineInfo &it, RVA offset) { return it.first < offset; });
        if (res == std::end(lineOffsets)) {
            return lineOffsets.back().second + 1;
        }
        return res->second;
    };

    auto fitsInScreen = [&](const Arrow &a) { return maxViewOffset - minViewOffset < a.length(); };

    std::sort(std::begin(arrows), std::end(arrows), [&](const Arrow &l, const Arrow &r) {
        int lScreen = fitsInScreen(l), rScreen = fitsInScreen(r);
        if (lScreen != rScreen) {
            return lScreen < rScreen;
        }
        return l.max != r.max ? l.max < r.max : l.min > r.min;
    });

    int minLine = 0, maxLine = 0;
    for (auto &it : arrows) {
        minLine = std::min(offsetToLine(it.min), minLine);
        maxLine = std::max(offsetToLine(it.max), maxLine);
        it.level = 0;
    }

    const int MAX_ARROW_LINES = 1 << 18;
    uint32_t maxLevel = 0;
    if (!arrows.empty() && maxLine - minLine < MAX_ARROW_LINES) {
        // Limit maximum tree range to MAX_ARROW_LINES as sanity check, since the tree is designed
        // for dense ranges. Under normal conditions due to amount lines fitting screen and number
        // of arrows remembered should be few hundreds at most.
        MinMaxAccumulateTree<uint32_t> maxLevelTree(maxLine - minLine + 2);
        for (Arrow &arrow : arrows) {
            int top = offsetToLine(arrow.min) - minLine;
            int bottom = offsetToLine(arrow.max) - minLine + 1;
            auto minMax = maxLevelTree.rangeMinMax(top, bottom);
            if (minMax.first > 1) {
                arrow.level = 1; // place below existing lines
            } else {
                arrow.level = minMax.second + 1; // place on top of existing lines
                maxLevel = std::max(maxLevel, arrow.level);
            }
            maxLevelTree.updateRange(top, bottom, arrow.level);
        }
    }

    const RVA currOffset = disas->getSeekable()->getOffset();
    const qreal pixelRatio = qhelpers::devicePixelRatio(p.device());
    const Arrow visibleRange { lines.first().offset, lines.last().offset };
    // Draw the lines
    for (const auto &arrow : arrows) {
        if (!visibleRange.intersects(arrow)) {
            continue;
        }
        int lineOffset =
                int((distanceBetweenLines * arrow.level + distanceBetweenLines) * pixelRatio);

        p.setPen(arrow.up ? penUp : penDown);
        if (arrow.min == currOffset || arrow.max == currOffset) {
            QPen pen = p.pen();
            pen.setWidthF((penSizePix * 3) / 2.0);
            p.setPen(pen);
        }

        auto lineToPixels = [&](int i) {
            int offset = int(arrow.up ? std::floor(pixelRatio) : -std::floor(pixelRatio));
            int clampedLine = std::max(0, std::min(i, ((int)lineY.size()) - 1));
            int pos0 = 0;
            if (lineY.size() > 0) {
                pos0 = lineY[clampedLine];
            }
            return pos0 + (i - clampedLine) * lineHeight + lineHeight / 2 + offset;
        };

        int lineStartNumber = offsetToLine(arrow.jmpFromOffset());
        int currentLineYPos = lineToPixels(lineStartNumber);

        int arrowLineNumber = offsetToLine(arrow.jmpToffset());
        int lineArrowY = lineToPixels(arrowLineNumber);

        if (lineStartNumber == arrowLineNumber) {
            currentLineYPos += lineHeight / 4;
            lineArrowY -= lineHeight / 4;
        }

        // Draw the lines
        p.drawLine(rightOffset, currentLineYPos, rightOffset - lineOffset, currentLineYPos); // left
        p.drawLine(rightOffset - lineOffset, currentLineYPos, rightOffset - lineOffset,
                   lineArrowY); // vertical

        p.drawLine(rightOffset - lineOffset, lineArrowY, rightOffset, lineArrowY); // right

        { // triangle
            QPainterPath arrow;
            arrow.moveTo(rightOffset - arrowWidth, lineArrowY + arrowWidth);
            arrow.lineTo(rightOffset - arrowWidth, lineArrowY - arrowWidth);
            arrow.lineTo(rightOffset, lineArrowY);
            p.fillPath(arrow, p.pen().brush());
        }
    }

    if (maxLevel > maxLevelBeforeFlush) {
        arrows.clear();
    }

    const size_t eraseN = arrows.size() > arrowsSize ? arrows.size() - arrowsSize : 0;
    if (eraseN > 0) {
        const bool scrolledDown = lastBeginOffset > lines.first().offset;
        std::sort(std::begin(arrows), std::end(arrows), [&](const Arrow &l, const Arrow &r) {
            if (scrolledDown) {
                return l.jmpFromOffset() < r.jmpFromOffset();
            } else {
                return l.jmpFromOffset() > r.jmpFromOffset();
            }
        });
        arrows.erase(std::end(arrows) - eraseN, std::end(arrows));
    }

    lastBeginOffset = lines.first().offset;
}

void DisassemblyLeftPanel::clearArrowFrom(RVA offset)
{
    auto it = std::find_if(arrows.begin(), arrows.end(),
                           [&](const Arrow &it) { return it.jmpFromOffset() == offset; });
    if (it != arrows.end()) {
        arrows.erase(it);
    }
}
