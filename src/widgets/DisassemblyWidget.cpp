#include "DisassemblyWidget.h"
#include "menus/DisassemblyContextMenu.h"
#include "common/Configuration.h"
#include "common/Helpers.h"
#include "common/TempConfig.h"
#include "common/SelectionHighlight.h"
#include "common/BinaryTrees.h"
#include "core/MainWindow.h"

#include <QApplication>
#include <QScrollBar>
#include <QJsonArray>
#include <QJsonObject>
#include <QVBoxLayout>
#include <QRegularExpression>
#include <QToolTip>
#include <QTextBlockUserData>
#include <QPainter>
#include <QPainterPath>
#include <QSplitter>

#include <algorithm>
#include <cmath>

class DisassemblyTextBlockUserData : public QTextBlockUserData
{
public:
    DisassemblyLine line;

    explicit DisassemblyTextBlockUserData(const DisassemblyLine &line) { this->line = line; }
};

static DisassemblyTextBlockUserData *getUserData(const QTextBlock &block)
{
    QTextBlockUserData *userData = block.userData();
    if (!userData) {
        return nullptr;
    }

    return static_cast<DisassemblyTextBlockUserData *>(userData);
}

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
    mDisasScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
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
    connect(Core(), SIGNAL(functionsChanged()), this, SLOT(refreshDisasm()));
    connect(Core(), &CutterCore::functionRenamed, this, [this]() { refreshDisasm(); });
    connect(Core(), SIGNAL(varsChanged()), this, SLOT(refreshDisasm()));
    connect(Core(), SIGNAL(asmOptionsChanged()), this, SLOT(refreshDisasm()));
    connect(Core(), &CutterCore::instructionChanged, this, &DisassemblyWidget::refreshIfInRange);
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

#define ADD_ACTION(ksq, ctx, slot)                                                                 \
    {                                                                                              \
        QAction *a = new QAction(this);                                                            \
        a->setShortcut(ksq);                                                                       \
        a->setShortcutContext(ctx);                                                                \
        addAction(a);                                                                              \
        connect(a, &QAction::triggered, this, (slot));                                             \
    }

    // Space to switch to graph
    ADD_ACTION(Qt::Key_Space, Qt::WidgetWithChildrenShortcut,
               [this] { mainWindow->showMemoryWidget(MemoryWidgetType::Graph); })

    ADD_ACTION(Qt::Key_Escape, Qt::WidgetWithChildrenShortcut, &DisassemblyWidget::seekPrev)

    ADD_ACTION(Qt::Key_J, Qt::WidgetWithChildrenShortcut,
               [this]() { moveCursorRelative(false, false); })
    ADD_ACTION(QKeySequence::MoveToNextLine, Qt::WidgetWithChildrenShortcut,
               [this]() { moveCursorRelative(false, false); })
    ADD_ACTION(Qt::Key_K, Qt::WidgetWithChildrenShortcut,
               [this]() { moveCursorRelative(true, false); })
    ADD_ACTION(QKeySequence::MoveToPreviousLine, Qt::WidgetWithChildrenShortcut,
               [this]() { moveCursorRelative(true, false); })
    ADD_ACTION(QKeySequence::MoveToNextPage, Qt::WidgetWithChildrenShortcut,
               [this]() { moveCursorRelative(false, true); })
    ADD_ACTION(QKeySequence::MoveToPreviousPage, Qt::WidgetWithChildrenShortcut,
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

QFontMetrics DisassemblyWidget::getFontMetrics()
{
    return mDisasTextEdit->fontMetrics();
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
        RVA lineOffset = readDisassemblyOffset(highlightSelection.cursor);
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
            RVA lineOffset = readDisassemblyOffset(highlightSelection.cursor);
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
    return readDisassemblyOffset(tc);
}

RVA DisassemblyWidget::readDisassemblyOffset(QTextCursor tc)
{
    auto userData = getUserData(tc.block());
    if (!userData) {
        return RVA_INVALID;
    }

    return userData->line.offset;
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
            RVA lineOffset = readDisassemblyOffset(cursor);
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
        if (readDisassemblyOffset(c) != offset) {
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
    RVA offset = readDisassemblyOffset(cursor);
    seekable->seekToReference(offset);
}

bool DisassemblyWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonDblClick
        && (obj == mDisasTextEdit || obj == mDisasTextEdit->viewport())) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

        const QTextCursor &cursor = mDisasTextEdit->cursorForPosition(mouseEvent->pos());
        jumpToOffsetUnderCursor(cursor);

        return true;
    } else if (event->type() == QEvent::ToolTip && obj == mDisasTextEdit->viewport()) {
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);

        auto cursorForWord = mDisasTextEdit->cursorForPosition(helpEvent->pos());
        cursorForWord.select(QTextCursor::WordUnderCursor);

        RVA offsetFrom = readDisassemblyOffset(cursorForWord);
        RVA offsetTo = RVA_INVALID;

        QList<XrefDescription> refs = Core()->getXRefs(offsetFrom, false, false);

        if (refs.length()) {
            if (refs.length() > 1) {
                qWarning() << tr("More than one (%1) references here. Weird behaviour expected.")
                                      .arg(refs.length());
            }
            offsetTo = refs.at(0).to; // This is the offset we want to preview

            if (Q_UNLIKELY(offsetFrom != refs.at(0).from)) {
                qWarning() << tr("offsetFrom (%1) differs from refs.at(0).from (%(2))")
                                      .arg(offsetFrom)
                                      .arg(refs.at(0).from);
            }

            // Only if the offset we point *to* is different from the one the cursor is currently
            // on *and* the former is a valid offset, we are allowed to get a preview of offsetTo
            if (offsetTo != offsetFrom && offsetTo != RVA_INVALID) {
                QStringList disasmPreview = Core()->getDisassemblyPreview(offsetTo, 10);

                // Last check to make sure the returned preview isn't an empty text (QStringList)
                if (!disasmPreview.isEmpty()) {
                    const QFont &fnt = Config()->getFont();
                    QFontMetrics fm { fnt };

                    QString tooltip =
                            QString("<html><div style=\"font-family: %1; font-size: %2pt; "
                                    "white-space: nowrap;\"><div style=\"margin-bottom: "
                                    "10px;\"><strong>Disassembly Preview</strong>:<br>%3<div>")
                                    .arg(fnt.family())
                                    .arg(qMax(6, fnt.pointSize() - 1))
                                    .arg(disasmPreview.join("<br>"));
                    QToolTip::showText(helpEvent->globalPos(), tooltip, this, QRect(), 3500);
                }
            }
        }

        return true;
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

void DisassemblyWidget::on_seekChanged(RVA offset)
{
    if (!seekFromCursor) {
        cursorLineOffset = 0;
        cursorCharOffset = 0;
    }

    if (topOffset != RVA_INVALID && offset >= topOffset && offset <= bottomOffset) {
        // if the line with the seek offset is currently visible, just move the cursor there
        updateCursorPosition();
    } else {
        // otherwise scroll there
        refreshDisasm(offset);
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
}

DisassemblyScrollArea::DisassemblyScrollArea(QWidget *parent) : QAbstractScrollArea(parent) {}

bool DisassemblyScrollArea::viewportEvent(QEvent *event)
{
    int dy = verticalScrollBar()->value() - 5;
    if (dy != 0) {
        emit scrollLines(dy);
    }

    if (event->type() == QEvent::Resize) {
        emit disassemblyResized();
    }

    resetScrollBars();
    return QAbstractScrollArea::viewportEvent(event);
}

void DisassemblyScrollArea::resetScrollBars()
{
    verticalScrollBar()->blockSignals(true);
    verticalScrollBar()->setRange(0, 10);
    verticalScrollBar()->setValue(5);
    verticalScrollBar()->blockSignals(false);
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
    int topOffset = int(tEdit->contentsMargins().top() + tEdit->textOffset());
    int lineHeight = disas->getFontMetrics().height();
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
                arrow.level = 1; // place bellow existing lines
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
            return i * lineHeight + lineHeight / 2 + topOffset + offset;
        };

        int lineStartNumber = offsetToLine(arrow.jmpFromOffset());
        int currentLineYPos = lineToPixels(lineStartNumber);

        int arrowLineNumber = offsetToLine(arrow.jmpToffset());
        int lineArrowY = lineToPixels(arrowLineNumber);

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
