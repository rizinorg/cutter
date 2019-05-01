#include "DisassemblyWidget.h"
#include "menus/DisassemblyContextMenu.h"
#include "common/Configuration.h"
#include "common/Helpers.h"
#include "common/TempConfig.h"

#include <QScrollBar>
#include <QJsonArray>
#include <QJsonObject>
#include <QVBoxLayout>
#include <QRegularExpression>
#include <QTextBlockUserData>


class DisassemblyTextBlockUserData: public QTextBlockUserData
{
public:
    DisassemblyLine line;

    explicit DisassemblyTextBlockUserData(const DisassemblyLine &line)
    {
        this->line = line;
    }
};

static DisassemblyTextBlockUserData *getUserData(const QTextBlock &block)
{
    QTextBlockUserData *userData = block.userData();
    if (!userData) {
        return nullptr;
    }

    return static_cast<DisassemblyTextBlockUserData *>(userData);
}

DisassemblyWidget::DisassemblyWidget(MainWindow *main, QAction *action)
    :   MemoryDockWidget(CutterCore::MemoryWidgetType::Disassembly, main, action)
    ,   mCtxMenu(new DisassemblyContextMenu(this))
    ,   mDisasScrollArea(new DisassemblyScrollArea(this))
    ,   mDisasTextEdit(new DisassemblyTextEdit(this))
    ,   seekable(new CutterSeekable(this))
{
    /*
     * Ugly hack just for the layout issue
     * QSettings saves the state with the object names
     * By doing this hack,
     * you can at least avoid some mess by dismissing all the Extra Widgets
     */
    QString name = "Disassembly";
    if (!action) {
        name = "Extra Disassembly";
    }
    setObjectName(name);

    topOffset = bottomOffset = RVA_INVALID;
    cursorLineOffset = 0;
    cursorCharOffset = 0;
    seekFromCursor = false;

    setWindowTitle(tr("Disassembly"));

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(mDisasTextEdit);
    layout->setMargin(0);
    mDisasScrollArea->viewport()->setLayout(layout);
    mDisasScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setWidget(mDisasScrollArea);

    setAllowedAreas(Qt::AllDockWidgetAreas);

    setupFonts();
    setupColors();

    disasmRefresh = createReplacingRefreshDeferrer<RVA>(false, [this](const RVA *offset) {
        refreshDisasm(offset ? *offset : RVA_INVALID);
    });

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
    mDisasTextEdit->viewport()->installEventFilter(this);

    // Set Disas context menu
    mDisasTextEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(mDisasTextEdit, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showDisasContextMenu(const QPoint &)));


    connect(mDisasScrollArea, SIGNAL(scrollLines(int)), this, SLOT(scrollInstructions(int)));
    connect(mDisasScrollArea, SIGNAL(disassemblyResized()), this, SLOT(updateMaxLines()));

    connectCursorPositionChanged(false);
    connect(mDisasTextEdit->verticalScrollBar(), &QScrollBar::valueChanged, this, [ = ](int value) {
        if (value != 0) {
            mDisasTextEdit->verticalScrollBar()->setValue(0);
        }
    });

    connect(Core(), SIGNAL(commentsChanged()), this, SLOT(refreshDisasm()));
    connect(Core(), SIGNAL(flagsChanged()), this, SLOT(refreshDisasm()));
    connect(Core(), SIGNAL(functionsChanged()), this, SLOT(refreshDisasm()));
    connect(Core(), SIGNAL(functionRenamed(const QString &, const QString &)), this,
            SLOT(refreshDisasm()));
    connect(Core(), SIGNAL(varsChanged()), this, SLOT(refreshDisasm()));
    connect(Core(), SIGNAL(asmOptionsChanged()), this, SLOT(refreshDisasm()));
    connect(Core(), &CutterCore::instructionChanged, this, [this](RVA offset) {
        if (offset >= topOffset && offset <= bottomOffset) {
            refreshDisasm();
        }
    });
    connect(Core(), SIGNAL(refreshCodeViews()), this, SLOT(refreshDisasm()));

    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(fontsUpdatedSlot()));
    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(colorsUpdatedSlot()));

    connect(this, &QDockWidget::visibilityChanged, this, [](bool visibility) {
        bool emptyGraph = (Core()->getMemoryWidgetPriority() == CutterCore::MemoryWidgetType::Graph
                           && Core()->isGraphEmpty());
        if (visibility && !emptyGraph) {
            Core()->setMemoryWidgetPriority(CutterCore::MemoryWidgetType::Disassembly);
        }
    });

    connect(Core(), &CutterCore::refreshAll, this, [this]() {
        refreshDisasm(seekable->getOffset());
    });
    refreshDisasm(seekable->getOffset());

    connect(mCtxMenu, SIGNAL(copy()), mDisasTextEdit, SLOT(copy()));

    mCtxMenu->addSeparator();
    syncIt.setText(tr("Sync/unsync offset"));
    mCtxMenu->addAction(&syncIt);
    connect(&syncIt, SIGNAL(triggered(bool)), this, SLOT(toggleSync()));
    connect(seekable, &CutterSeekable::seekableSeekChanged, this, &DisassemblyWidget::on_seekChanged);

    addActions(mCtxMenu->actions());

#define ADD_ACTION(ksq, ctx, slot) {\
    QAction *a = new QAction(this); \
    a->setShortcut(ksq); \
    a->setShortcutContext(ctx); \
    addAction(a); \
    connect(a, &QAction::triggered, this, (slot)); }

    // Space to switch to graph
    ADD_ACTION(Qt::Key_Space, Qt::WidgetWithChildrenShortcut, [] {
        Core()->setMemoryWidgetPriority(CutterCore::MemoryWidgetType::Graph);
        Core()->triggerRaisePrioritizedMemoryWidget();
    })

    ADD_ACTION(Qt::Key_Escape, Qt::WidgetWithChildrenShortcut, &DisassemblyWidget::seekPrev)

    ADD_ACTION(Qt::Key_J, Qt::WidgetWithChildrenShortcut, [this]() {
        moveCursorRelative(false, false);
    })
    ADD_ACTION(QKeySequence::MoveToNextLine, Qt::WidgetWithChildrenShortcut, [this]() {
        moveCursorRelative(false, false);
    })
    ADD_ACTION(Qt::Key_K, Qt::WidgetWithChildrenShortcut, [this]() {
        moveCursorRelative(true, false);
    })
    ADD_ACTION(QKeySequence::MoveToPreviousLine, Qt::WidgetWithChildrenShortcut, [this]() {
        moveCursorRelative(true, false);
    })
    ADD_ACTION(QKeySequence::MoveToNextPage, Qt::WidgetWithChildrenShortcut, [this]() {
        moveCursorRelative(false, true);
    })
    ADD_ACTION(QKeySequence::MoveToPreviousPage, Qt::WidgetWithChildrenShortcut, [this]() {
        moveCursorRelative(true, true);
    })
    ADD_ACTION(QKeySequence(Qt::CTRL + Qt::Key_Equal), Qt::WidgetWithChildrenShortcut, &DisassemblyWidget::zoomIn)
    ADD_ACTION(QKeySequence(Qt::CTRL + Qt::Key_Minus), Qt::WidgetWithChildrenShortcut, &DisassemblyWidget::zoomOut)
#undef ADD_ACTION
}

void DisassemblyWidget::toggleSync()
{
    QString windowTitle = tr("Disassembly");
    seekable->toggleSynchronization();
    if (seekable->isSynchronized()) {
        setWindowTitle(windowTitle);
    } else {
        setWindowTitle(windowTitle + CutterSeekable::tr(" (unsynced)"));
    }
}

void DisassemblyWidget::setPreviewMode(bool previewMode)
{
    mDisasTextEdit->setContextMenuPolicy(previewMode
                                         ? Qt::NoContextMenu
                                         : Qt::CustomContextMenu);
    mCtxMenu->setEnabled(!previewMode);
    for (auto action : mCtxMenu->actions()) {
        action->setEnabled(!previewMode);
    }
    for (auto action : actions()) {
        if (action->shortcut() == Qt::Key_Space ||
            action->shortcut() == Qt::Key_Escape) {
            action->setEnabled(!previewMode);
        }
    }
    if (seekable->isSynchronized() && previewMode) {
        toggleSync();
    }
}

QWidget *DisassemblyWidget::getTextWidget()
{
    return mDisasTextEdit;
}

void DisassemblyWidget::refreshDisasm(RVA offset)
{
    if(!disasmRefresh->attemptRefresh(offset == RVA_INVALID ? nullptr : new RVA(offset))) {
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

    QList<DisassemblyLine> disassemblyLines;
    {
        TempConfig tempConfig;
        tempConfig.set("scr.color", COLOR_MODE_16M);
        disassemblyLines = Core()->disassembleLines(topOffset, maxLines);
    }

    connectCursorPositionChanged(true);

    mDisasTextEdit->document()->clear();
    QTextCursor cursor(mDisasTextEdit->document());
    QTextBlockFormat regular = cursor.blockFormat();
    for (const DisassemblyLine &line : disassemblyLines) {
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

    if (!disassemblyLines.isEmpty()) {
        bottomOffset = disassemblyLines[qMin(disassemblyLines.size(), maxLines) - 1].offset;
        if (bottomOffset < topOffset) {
            bottomOffset = RVA_MAX;
        }
    } else {
        bottomOffset = topOffset;
    }

    // remove additional lines
    QTextCursor tc = mDisasTextEdit->textCursor();
    tc.movePosition(QTextCursor::Start);
    tc.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, maxLines - 1);
    tc.movePosition(QTextCursor::EndOfLine);
    tc.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    tc.removeSelectedText();

    connectCursorPositionChanged(false);

    updateCursorPosition();

    mDisasTextEdit->setLockScroll(false);
    mDisasTextEdit->horizontalScrollBar()->setValue(horizontalScrollValue);
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

void DisassemblyWidget::zoomIn()
{
    mDisasTextEdit->zoomIn();
    updateMaxLines();
}

void DisassemblyWidget::zoomOut()
{
    mDisasTextEdit->zoomOut();
    updateMaxLines();
}

void DisassemblyWidget::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    QColor highlightColor = ConfigColor("linehl");
    QColor highlightPCColor = ConfigColor("highlightPC");

    // Highlight the current word
    QTextCursor cursor = mDisasTextEdit->textCursor();
    cursor.select(QTextCursor::WordUnderCursor);
    QString searchString = cursor.selectedText();
    curHighlightedWord = searchString;

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
    extraSelections.append(getSameWordsSelections());

    // highlight PC line
    RVA PCAddr = Core()->getProgramCounterValue();
    highlightSelection.cursor = cursor;
    highlightSelection.cursor.movePosition(QTextCursor::Start);
    if (PCAddr != RVA_INVALID) {
        while (true) {
            RVA lineOffset = readDisassemblyOffset(highlightSelection.cursor);
            if (lineOffset == PCAddr) {
                highlightSelection.format.setBackground(highlightPCColor);
                highlightSelection.format.setProperty(QTextFormat::FullWidthSelection, true);
                highlightSelection.cursor.clearSelection();
                extraSelections.append(highlightSelection);
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

    mDisasTextEdit->setExtraSelections(extraSelections);
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
        mDisasTextEdit->setExtraSelections(getSameWordsSelections());
    } else {
        RVA currentCursorOffset = readCurrentDisassemblyOffset();
        QTextCursor originalCursor = mDisasTextEdit->textCursor();

        QTextCursor cursor = originalCursor;
        cursor.movePosition(QTextCursor::Start);

        while (true) {
            RVA lineOffset = readDisassemblyOffset(cursor);
            if (lineOffset == offset) {
                if (cursorLineOffset > 0) {
                    cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, cursorLineOffset);
                }
                if (cursorCharOffset > 0) {
                    cursor.movePosition(QTextCursor::StartOfLine);
                    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, cursorCharOffset);
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
    connectCursorPositionChanged(false);
}

void DisassemblyWidget::connectCursorPositionChanged(bool disconnect)
{
    if (disconnect) {
        QObject::disconnect(mDisasTextEdit, SIGNAL(cursorPositionChanged()), this,
                            SLOT(cursorPositionChanged()));
    } else {
        connect(mDisasTextEdit, SIGNAL(cursorPositionChanged()), this, SLOT(cursorPositionChanged()));
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
    mCtxMenu->setCanCopy(mDisasTextEdit->textCursor().hasSelection());
    if (mDisasTextEdit->textCursor().hasSelection()) {
        // A word is selected so use it
        mCtxMenu->setCurHighlightedWord(mDisasTextEdit->textCursor().selectedText());
    } else {
        // No word is selected so use the word under the cursor
        mCtxMenu->setCurHighlightedWord(curHighlightedWord);
    }
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
        }
    }
}

QList<QTextEdit::ExtraSelection> DisassemblyWidget::getSameWordsSelections()
{
    QList<QTextEdit::ExtraSelection> selections;
    QTextEdit::ExtraSelection highlightSelection;
    QTextDocument *document = mDisasTextEdit->document();
    QColor highlightWordColor = ConfigColor("wordhl");

    if (curHighlightedWord.isNull()) {
        return QList<QTextEdit::ExtraSelection>();
    }

    highlightSelection.cursor = mDisasTextEdit->textCursor();
    highlightSelection.cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);

    while (!highlightSelection.cursor.isNull() && !highlightSelection.cursor.atEnd()) {
        highlightSelection.cursor = document->find(curHighlightedWord, highlightSelection.cursor,
                                                   QTextDocument::FindWholeWords);

        if (!highlightSelection.cursor.isNull()) {
            highlightSelection.format.setBackground(highlightWordColor);

            selections.append(highlightSelection);
        }
    }
    return selections;
}

bool DisassemblyWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonDblClick
        && (obj == mDisasTextEdit || obj == mDisasTextEdit->viewport())) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

        QTextCursor cursor = mDisasTextEdit->cursorForPosition(QPoint(mouseEvent->x(), mouseEvent->y()));
        RVA offset = readDisassemblyOffset(cursor);

        RVA jump = Core()->getOffsetJump(offset);

        if (jump == RVA_INVALID) {
            bool ok;
            RVA xref = Core()->cmdj("axfj@" + QString::number(
                offset)).array().first().toObject().value("to").toVariant().toULongLong(&ok);
            if (ok) {
                jump = xref;
            }
        }

        if (jump != RVA_INVALID) {
            seekable->seek(jump);
        }

        return true;
    }
    return CutterDockWidget::eventFilter(obj, event);
}

void DisassemblyWidget::on_seekChanged(RVA offset)
{
    if (!seekFromCursor) {
        cursorLineOffset = 0;
        cursorCharOffset = 0;
    }

    if (topOffset != RVA_INVALID
            && offset >= topOffset && offset <= bottomOffset) {
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

DisassemblyScrollArea::DisassemblyScrollArea(QWidget *parent) : QAbstractScrollArea(parent)
{
}

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
    //QPlainTextEdit::keyPressEvent(event);
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
