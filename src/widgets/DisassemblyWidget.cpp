#include "DisassemblyWidget.h"
#include "menus/DisassemblyContextMenu.h"
#include "utils/HexAsciiHighlighter.h"
#include "utils/HexHighlighter.h"
#include "utils/Configuration.h"
#include "utils/Helpers.h"
#include "utils/TempConfig.h"

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
    :   CutterDockWidget(main, action)
    ,   mCtxMenu(new DisassemblyContextMenu(this))
    ,   mDisasScrollArea(new DisassemblyScrollArea(this))
    ,   mDisasTextEdit(new DisassemblyTextEdit(this))
    ,   seekable(new CutterSeekableWidget(this))
{
    topOffset = bottomOffset = RVA_INVALID;
    cursorLineOffset = 0;
    seekFromCursor = false;

    setWindowTitle(tr("Disassembly"));

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(mDisasTextEdit);
    layout->setMargin(0);
    mDisasScrollArea->viewport()->setLayout(layout);
    mDisasScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setWidget(mDisasScrollArea);

    setAllowedAreas(Qt::AllDockWidgetAreas);
    setObjectName("DisassemblyWidget");

    setupFonts();
    setupColors();

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


    // Space to switch to graph
    QShortcut *graphShortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);
    graphShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(graphShortcut, &QShortcut::activated, this, [] {
        Core()->setMemoryWidgetPriority(CutterCore::MemoryWidgetType::Graph);
        Core()->triggerRaisePrioritizedMemoryWidget();
    });


    connect(mDisasScrollArea, SIGNAL(scrollLines(int)), this, SLOT(scrollInstructions(int)));
    connect(mDisasScrollArea, SIGNAL(disassemblyResized()), this, SLOT(updateMaxLines()));

    connectCursorPositionChanged(false);
    connect(mDisasTextEdit->verticalScrollBar(), &QScrollBar::valueChanged, this, [ = ](int value) {
        if (value != 0) {
            mDisasTextEdit->verticalScrollBar()->setValue(0);
        }
    });

    connect(Core(), SIGNAL(raisePrioritizedMemoryWidget(CutterCore::MemoryWidgetType)), this,
            SLOT(raisePrioritizedMemoryWidget(CutterCore::MemoryWidgetType)));
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
        if (visibility) {
            Core()->setMemoryWidgetPriority(CutterCore::MemoryWidgetType::Disassembly);
        }
    });

    connect(Core(), &CutterCore::refreshAll, this, [this]() {
        refreshDisasm(seekable->getOffset());
    });

    connect(mCtxMenu, SIGNAL(copy()), mDisasTextEdit, SLOT(copy()));

    // Dirty
    QShortcut *shortcut_escape = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    shortcut_escape->setContext(Qt::WidgetShortcut);
    connect(shortcut_escape, SIGNAL(activated()), this, SLOT(seekPrev()));

    mCtxMenu->addSeparator();
    syncIt.setText(tr("Sync/unsync offset"));
    mCtxMenu->addAction(&syncIt);
    connect(&syncIt, SIGNAL(triggered(bool)), this, SLOT(toggleSync()));
    connect(seekable, &CutterSeekableWidget::seekChanged, this, &DisassemblyWidget::on_seekChanged);

#define ADD_SHORTCUT(ksq, slot) { \
    QShortcut *s = new QShortcut((ksq), this); \
    s->setContext(Qt::WidgetShortcut); \
    connect(s, &QShortcut::activated, this, (slot)); \
}
    ADD_SHORTCUT(QKeySequence(Qt::Key_J), [this]() {
        moveCursorRelative(false, false);
    })
    ADD_SHORTCUT(QKeySequence::MoveToNextLine, [this]() {
        moveCursorRelative(false, false);
    })
    ADD_SHORTCUT(QKeySequence(Qt::Key_K), [this]() {
        moveCursorRelative(true, false);
    })
    ADD_SHORTCUT(QKeySequence::MoveToPreviousLine, [this]() {
        moveCursorRelative(true, false);
    })
    ADD_SHORTCUT(QKeySequence::MoveToNextPage, [this]() {
        moveCursorRelative(false, true);
    })
    ADD_SHORTCUT(QKeySequence::MoveToPreviousPage, [this]() {
        moveCursorRelative(true, true);
    })
    ADD_SHORTCUT(QKeySequence(Qt::CTRL + Qt::Key_Plus), &DisassemblyWidget::zoomIn)
    ADD_SHORTCUT(QKeySequence(Qt::CTRL + Qt::Key_Minus), &DisassemblyWidget::zoomOut)
#undef ADD_SHORTCUT
}

void DisassemblyWidget::toggleSync()
{
    QString windowTitle = tr("Disassembly");
    seekable->toggleSyncWithCore();
    if (seekable->getSyncWithCore()) {
        setWindowTitle(windowTitle);
    } else {
        setWindowTitle(windowTitle + " (not synced)");
        seekable->setIndependentOffset(Core()->getOffset());
    }
}

QWidget *DisassemblyWidget::getTextWidget()
{
    return mDisasTextEdit;
}

void DisassemblyWidget::refreshDisasm(RVA offset)
{
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

    int horizontalScrollValue = mDisasTextEdit->horizontalScrollBar()->value();
    mDisasTextEdit->setLockScroll(true); // avoid flicker

    QList<DisassemblyLine> disassemblyLines;
    {
        TempConfig tempConfig;
        tempConfig.set("scr.html", true)
        .set("scr.color", COLOR_MODE_16M);
        disassemblyLines = Core()->disassembleLines(topOffset, maxLines);
    }

    connectCursorPositionChanged(true);

    mDisasTextEdit->document()->clear();
    QTextCursor cursor(mDisasTextEdit->document());
    for (DisassemblyLine line : disassemblyLines) {
        if (line.offset < topOffset) { // overflow
            break;
        }
        cursor.insertHtml(line.text);
        auto a = new DisassemblyTextBlockUserData(line);
        cursor.block().setUserData(a);
        cursor.insertBlock();
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

void DisassemblyWidget::zoomIn() {
    mDisasTextEdit->zoomIn();
    updateMaxLines();
}

void DisassemblyWidget::zoomOut() {
    mDisasTextEdit->zoomOut();
    updateMaxLines();
}

void DisassemblyWidget::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    QColor highlightColor = ConfigColor("highlight");
    QColor highlightPCColor = ConfigColor("highlightPC");
    QColor highlightWordColor = ConfigColor("highlightWord");
    highlightWordColor.setAlpha(128);
    QColor highlightWordCurrentLineColor = ConfigColor("gui.background");
    highlightWordCurrentLineColor.setAlpha(128);

    // Highlight the current line
    QTextEdit::ExtraSelection selection;
    selection.format.setBackground(highlightColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = mDisasTextEdit->textCursor();
    selection.cursor.clearSelection();
    extraSelections.append(selection);

    // Highlight the current word
    QTextCursor cursor = mDisasTextEdit->textCursor();
    cursor.select(QTextCursor::WordUnderCursor);
    QString searchString = cursor.selectedText();

    cursor.movePosition(QTextCursor::StartOfLine);
    int listStartPos = cursor.position();
    cursor.movePosition(QTextCursor::EndOfLine);
    int lineEndPos = cursor.position();

    // Highlight all the words in the document same as the current one
    QTextDocument *document = mDisasTextEdit->document();

    QTextEdit::ExtraSelection highlightSelection;
    highlightSelection.cursor = cursor;
    highlightSelection.cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);

    while (!highlightSelection.cursor.isNull() && !highlightSelection.cursor.atEnd()) {
        highlightSelection.cursor = document->find(searchString, highlightSelection.cursor,
                                                   QTextDocument::FindWholeWords);

        if (!highlightSelection.cursor.isNull()) {
            if (highlightSelection.cursor.position() >= listStartPos
                    && highlightSelection.cursor.position() <= lineEndPos) {
                highlightSelection.format.setBackground(highlightWordCurrentLineColor);
            } else {
                highlightSelection.format.setBackground(highlightWordColor);
            }

            highlightSelection.cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
            extraSelections.append(highlightSelection);
        }
    }

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
        mDisasTextEdit->setExtraSelections({});
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

bool DisassemblyWidget::eventFilter(QObject *obj, QEvent *event)
{
    if ((obj == mDisasTextEdit || obj == mDisasTextEdit->viewport())
            && event->type() == QEvent::MouseButtonDblClick) {
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
    return QDockWidget::eventFilter(obj, event);
}

void DisassemblyWidget::on_seekChanged(RVA offset)
{
    if (!seekFromCursor) {
        cursorLineOffset = 0;
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

void DisassemblyWidget::raisePrioritizedMemoryWidget(CutterCore::MemoryWidgetType type)
{
    if (type == CutterCore::MemoryWidgetType::Disassembly) {
        raise();
        setFocus();
    }
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

void DisassemblyTextEdit::keyPressEvent(QKeyEvent */*event*/)
{
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
