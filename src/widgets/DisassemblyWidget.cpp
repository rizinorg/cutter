#include "DisassemblyWidget.h"
#include "menus/DisassemblyContextMenu.h"
#include "common/Configuration.h"
#include "common/Helpers.h"
#include "common/TempConfig.h"
#include "common/SelectionHighlight.h"
#include "core/MainWindow.h"

#include <QApplication>
#include <QScrollBar>
#include <QJsonArray>
#include <QJsonObject>
#include <QVBoxLayout>
#include <QRegularExpression>
#include <QTextBlockUserData>
#include <QPainter>
#include <QSplitter>


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
    :   MemoryDockWidget(MemoryWidgetType::Disassembly, main, action)
    ,   mCtxMenu(new DisassemblyContextMenu(this, main))
    ,   mDisasScrollArea(new DisassemblyScrollArea(this))
    ,   mDisasTextEdit(new DisassemblyTextEdit(this))
{
    setObjectName(main
                  ? main->getUniqueObjectName(getWidgetType())
                  : getWidgetType());

    topOffset = bottomOffset = RVA_INVALID;
    cursorLineOffset = 0;
    cursorCharOffset = 0;
    seekFromCursor = false;

    setWindowTitle(getWindowTitle());

    // Instantiate the window layout
    auto *splitter = new QSplitter;

    // Setup the left frame that contains breakpoints and jumps
    leftPanel = new DisassemblyLeftPanel(this);
    splitter->addWidget(leftPanel);

    // Setup the disassembly content
    auto *layout = new QHBoxLayout;
    layout->addWidget(mDisasTextEdit);
    layout->setMargin(0);
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
    connect(qApp, &QApplication::focusChanged, this, [this](QWidget* , QWidget* now) {
        QColor borderColor = this == now
                             ? palette().color(QPalette::Highlight)
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

    connect(Core(), &CutterCore::refreshAll, this, [this]() {
        refreshDisasm(seekable->getOffset());
    });
    refreshDisasm(seekable->getOffset());

    connect(mCtxMenu, SIGNAL(copy()), mDisasTextEdit, SLOT(copy()));

    mCtxMenu->addSeparator();
    mCtxMenu->addAction(&syncAction);
    connect(seekable, &CutterSeekable::seekableSeekChanged, this, &DisassemblyWidget::on_seekChanged);

    addActions(mCtxMenu->actions());

#define ADD_ACTION(ksq, ctx, slot) {\
    QAction *a = new QAction(this); \
    a->setShortcut(ksq); \
    a->setShortcutContext(ctx); \
    addAction(a); \
    connect(a, &QAction::triggered, this, (slot)); }

    // Space to switch to graph
    ADD_ACTION(Qt::Key_Space, Qt::WidgetWithChildrenShortcut, [this] {
        mainWindow->showMemoryWidget(MemoryWidgetType::Graph);
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
#undef ADD_ACTION
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

    // Retrieve disassembly lines
    {
        TempConfig tempConfig;
        tempConfig.set("scr.color", COLOR_MODE_16M)
		.set("asm.lines", false);
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
    extraSelections.append(createSameWordsSelections(mDisasTextEdit, curHighlightedWord));

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
        mDisasTextEdit->setExtraSelections(createSameWordsSelections(mDisasTextEdit, curHighlightedWord));
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
        }
    }
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
    return MemoryDockWidget::eventFilter(obj, event);
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

qreal DisassemblyTextEdit::textOffset() const
{
    return (blockBoundingGeometry(document()->begin()).topLeft() +
            contentOffset()).y();
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

/*********************
 * Left panel
 *********************/

struct Range {
    Range(RVA v1, RVA v2)
        : from(v1), to(v2) { if (from > to) std::swap(from, to); }
    RVA from;
    RVA to;

    inline bool contains(const Range& other) const
    {
        return from <= other.from && to >= other.to;
    }

    inline bool contains(RVA point) const
    {
        return from <= point && to >= point;
    }
};

DisassemblyLeftPanel::DisassemblyLeftPanel(DisassemblyWidget *disas)
{
    this->disas = disas;
}

void DisassemblyLeftPanel::wheelEvent(QWheelEvent *event) {
    int count = -(event->angleDelta() / 15).y();
    count -= (count > 0 ? 5 : -5);

    this->disas->scrollInstructions(count);
}

void DisassemblyLeftPanel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    using namespace std;
    constexpr int penSizePix = 1;
    constexpr int distanceBetweenLines = 10;
    constexpr int arrowWidth = 5;
    int rightOffset = size().rwidth();
    auto tEdit = qobject_cast<DisassemblyTextEdit*>(disas->getTextWidget());
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

    QMap<RVA, int> linesPixPosition;
    QMap<RVA, pair<RVA, int>> arrowInfo; /* offset -> (arrow, layer of arrow) */
    int nLines = 0;
    for (const auto& line : lines) {
        linesPixPosition[line.offset] = nLines * lineHeight + lineHeight / 2 + topOffset;
        nLines++;
        if (line.arrow != RVA_INVALID) {
            arrowInfo.insert(line.offset, { line.arrow, -1 });
        }
    }

    for (auto it = arrowInfo.begin(); it != arrowInfo.end(); it++) {
        Range currRange = { it.key(), it.value().first };
        it.value().second = it.value().second == -1
                            ? 1
                            : it.value().second;
        for (auto innerIt = arrowInfo.begin(); innerIt != arrowInfo.end(); innerIt++) {
            if (innerIt == it) {
                continue;
            }
            Range innerRange = { innerIt.key(), innerIt.value().first };
            if (currRange.contains(innerRange) || currRange.contains(innerRange.from)) {
                it.value().second++;
            }
        }
    }

    // I'm sorry this loop below, but it is only way I see how to implement the feature
    while (true) {
        bool correction = false;
        bool correction2 = false;
        for (auto it = arrowInfo.begin(); it != arrowInfo.end(); it++) {
            int minDistance = INT32_MAX;
            Range currRange = { it.key(), it.value().first };
            for (auto innerIt = arrowInfo.begin(); innerIt != arrowInfo.end(); innerIt++) {
                if (innerIt == it) {
                    continue;
                }
                Range innerRange = { innerIt.key(), innerIt.value().first };
                if (it.value().second == innerIt.value().second &&
                    (currRange.contains(innerRange) || currRange.contains(innerRange.from))) {
                    it.value().second++;
                    correction = true;
                }
                int distance = it.value().second - innerIt.value().second;
                if (distance > 0 && distance < minDistance) {
                    minDistance = distance;
                }
            }
            if (minDistance > 1 && minDistance != INT32_MAX) {
                correction2 = true;
                it.value().second -= minDistance - 1;
            }
        }
        if (!correction && !correction2) {
            break;
        }
    }

    const RVA currOffset = disas->getSeekable()->getOffset();
    qreal pixelRatio = qhelpers::devicePixelRatio(p.device());
    // Draw the lines
    for (const auto& l : lines) {
        int lineOffset = int((distanceBetweenLines * arrowInfo[l.offset].second + distanceBetweenLines) *
                         pixelRatio);
        // Skip until we reach a line that jumps to a destination
        if (l.arrow == RVA_INVALID) {
            continue;
        }

        bool jumpDown = l.arrow > l.offset;
        p.setPen(jumpDown ? penDown : penUp);
        if (l.offset == currOffset || l.arrow == currOffset) {
            QPen pen = p.pen();
            pen.setWidthF((penSizePix * 3) / 2.0);
            p.setPen(pen);
        }
        bool endVisible = true;

        int currentLineYPos = linesPixPosition[l.offset];
        int lineArrowY = linesPixPosition.value(l.arrow, -1);

        if (lineArrowY == -1) {
            lineArrowY = jumpDown
                              ? geometry().bottom()
                              : 0;
            endVisible = false;
        }

        // Draw the lines
        p.drawLine(rightOffset, currentLineYPos, rightOffset - lineOffset, currentLineYPos);
        p.drawLine(rightOffset - lineOffset, currentLineYPos, rightOffset - lineOffset, lineArrowY);

        if (endVisible) {
            p.drawLine(rightOffset - lineOffset, lineArrowY, rightOffset, lineArrowY);

            QPainterPath arrow;
            arrow.moveTo(rightOffset - arrowWidth, lineArrowY + arrowWidth);
            arrow.lineTo(rightOffset - arrowWidth, lineArrowY - arrowWidth);
            arrow.lineTo(rightOffset, lineArrowY);
            p.fillPath(arrow, p.pen().brush());
        }
    }
}
