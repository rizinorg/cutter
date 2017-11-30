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


DisassemblyWidget::DisassemblyWidget(QWidget *parent)
    :   QDockWidget(parent)
    ,   mCtxMenu(new DisassemblyContextMenu(this))
    ,   mDisasScrollArea(new DisassemblyScrollArea(this))
    ,   mDisasTextEdit(new DisassemblyTextEdit(this))
{
    topOffset = bottomOffset = RVA_INVALID;

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
    connect(graphShortcut, &QShortcut::activated, this, []{
        Core()->setMemoryWidgetPriority(CutterCore::MemoryWidgetType::Graph);
        Core()->triggerRaisePrioritizedMemoryWidget();
    });


    connect(mDisasScrollArea, SIGNAL(scrollLines(int)), this, SLOT(scrollInstructions(int)));
    connect(mDisasScrollArea, SIGNAL(disassemblyResized()), this, SLOT(updateMaxLines()));

    connectCursorPositionChanged(false);
    connect(mDisasTextEdit->verticalScrollBar(), &QScrollBar::valueChanged, this, [=](int value) {
        if (value != 0)
        {
            mDisasTextEdit->verticalScrollBar()->setValue(0);
        }
    });

    connect(Core(), SIGNAL(seekChanged(RVA)), this, SLOT(on_seekChanged(RVA)));
    connect(Core(), SIGNAL(raisePrioritizedMemoryWidget(CutterCore::MemoryWidgetType)), this, SLOT(raisePrioritizedMemoryWidget(CutterCore::MemoryWidgetType)));
    connect(Core(), SIGNAL(commentsChanged()), this, SLOT(refreshDisasm()));
    connect(Core(), SIGNAL(flagsChanged()), this, SLOT(refreshDisasm()));
    connect(Core(), SIGNAL(functionRenamed(QString, QString)), this, SLOT(refreshDisasm()));
    connect(Core(), SIGNAL(varsChanged()), this, SLOT(refreshDisasm()));
    connect(Core(), SIGNAL(asmOptionsChanged()), this, SLOT(refreshDisasm()));
    connect(Core(), &CutterCore::instructionChanged, this, [this](RVA offset) {
        if (offset >= topOffset && offset <= bottomOffset)
        {
            refreshDisasm();
        }
    });

    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(fontsUpdatedSlot()));
    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(colorsUpdatedSlot()));

    connect(this, &QDockWidget::visibilityChanged, this, [](bool visibility) {
        if (visibility)
        {
            Core()->setMemoryWidgetPriority(CutterCore::MemoryWidgetType::Disassembly);
        }
    });

    connect(Core(), &CutterCore::refreshAll, this, [this]() {
        refreshDisasm(Core()->getOffset());
    });

    connect(mCtxMenu, SIGNAL(copy()), mDisasTextEdit, SLOT(copy()));

    // Dirty
    QShortcut *shortcut_escape = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    shortcut_escape->setContext(Qt::WidgetShortcut);
    connect(shortcut_escape, SIGNAL(activated()), this, SLOT(seekPrev()));
}

DisassemblyWidget::DisassemblyWidget(const QString &title, QWidget *parent) :
    DisassemblyWidget(parent)
{
    this->setWindowTitle(title);
}

QWidget* DisassemblyWidget::getTextWidget()
{
    return mDisasTextEdit;
}

QString DisassemblyWidget::readDisasm(const QString &cmd, bool stripLastNewline)
{
    TempConfig tempConfig;
    tempConfig.set("scr.html", true)
            .set("scr.color", true);

    QString disas = Core()->cmd(cmd);

    if (stripLastNewline)
    {
        // ugly hack to remove trailing newline
        static const auto trimBrRegExp = QRegularExpression("<br />$");
        disas = disas.remove(trimBrRegExp);
    }

    return disas.trimmed();
}


void DisassemblyWidget::refreshDisasm(RVA offset)
{
    if (offset != RVA_INVALID)
    {
        topOffset = offset;
    }

    if (maxLines <= 0)
    {
        connectCursorPositionChanged(true);
        mDisasTextEdit->clear();
        connectCursorPositionChanged(false);
        return;
    }

    int horizontalScrollValue = mDisasTextEdit->horizontalScrollBar()->value();
    mDisasTextEdit->setLockScroll(true); // avoid flicker

    QString disas = readDisasm("pd " + QString::number(maxLines) + "@" + QString::number(topOffset), true);

    connectCursorPositionChanged(true);

    mDisasTextEdit->document()->setHtml(disas);

    // get bottomOffset from last visible line.
    // because pd N may return more than N lines, move maxLines lines down from the top
    mDisasTextEdit->moveCursor(QTextCursor::Start);
    QTextCursor tc = mDisasTextEdit->textCursor();
    tc.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, maxLines - 1);
    mDisasTextEdit->setTextCursor(tc);

    connectCursorPositionChanged(false);

    bottomOffset = readCurrentDisassemblyOffset();
    if (bottomOffset == RVA_INVALID)
    {
        bottomOffset = topOffset;
    }

    // remove additional lines
    tc.movePosition(QTextCursor::EndOfLine);
    tc.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    tc.removeSelectedText();

    updateCursorPosition();

    mDisasTextEdit->setLockScroll(false);
    mDisasTextEdit->horizontalScrollBar()->setValue(horizontalScrollValue);
}


void DisassemblyWidget::scrollInstructions(int count)
{
    if (count == 0)
    {
        return;
    }

    RVA offset;
    if (count > 0)
    {
        offset = Core()->nextOpAddr(topOffset, count);
    }
    else
    {
        offset = Core()->prevOpAddr(topOffset, -count);
    }

    refreshDisasm(offset);
}


bool DisassemblyWidget::updateMaxLines()
{
    int currentMaxLines = qhelpers::getMaxFullyDisplayedLines(mDisasTextEdit);

    if (currentMaxLines != maxLines)
    {
        maxLines = currentMaxLines;
        refreshDisasm();
        return true;
    }

    return false;
}

void DisassemblyWidget::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    QColor highlightColor = ConfigColor("highlight");
    QColor highlightWordColor = ConfigColor("highlight");
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

    while (!highlightSelection.cursor.isNull() && !highlightSelection.cursor.atEnd())
    {
        highlightSelection.cursor = document->find(searchString, highlightSelection.cursor, QTextDocument::FindWholeWords);

        if (!highlightSelection.cursor.isNull())
        {
            if (highlightSelection.cursor.position() >= listStartPos && highlightSelection.cursor.position() <= lineEndPos)
            {
                highlightSelection.format.setBackground(highlightWordCurrentLineColor);
            }
            else
            {
                highlightSelection.format.setBackground(highlightWordColor);
            }

            highlightSelection.cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
            extraSelections.append(highlightSelection);
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
    // TODO: do this in a different way without parsing the disassembly text

    static const QRegularExpression offsetRegExp("^0x[0-9A-Fa-f]*");

    while (true)
    {
        tc.select(QTextCursor::LineUnderCursor);

        QString line = tc.selectedText();

        auto match = offsetRegExp.match(line);
        if (match.hasMatch())
        {
            return match.captured(0).toULongLong(nullptr, 16);
        }

        tc.movePosition(QTextCursor::StartOfLine);
        if (tc.atStart())
        {
            break;
        }

        tc.movePosition(QTextCursor::Up);
    }

    return RVA_INVALID;
}

void DisassemblyWidget::updateCursorPosition()
{
    RVA offset = Core()->getOffset();

    // already fine where it is?
    RVA currentLineOffset = readCurrentDisassemblyOffset();
    if (currentLineOffset == offset)
    {
        return;
    }

    connectCursorPositionChanged(true);

    if (offset < topOffset || (offset > bottomOffset && bottomOffset != RVA_INVALID))
    {
        mDisasTextEdit->moveCursor(QTextCursor::Start);
        mDisasTextEdit->setExtraSelections({});
    }
    else
    {
        RVA currentCursorOffset = readCurrentDisassemblyOffset();
        QTextCursor originalCursor = mDisasTextEdit->textCursor();

        QTextCursor cursor = originalCursor;
        cursor.movePosition(QTextCursor::Start);

        while (true)
        {
            RVA lineOffset = readDisassemblyOffset(cursor);
            if (lineOffset == offset)
            {
                mDisasTextEdit->setTextCursor(cursor);
                highlightCurrentLine();
                break;
            }
            else if (lineOffset != RVA_INVALID && lineOffset > offset)
            {
                mDisasTextEdit->moveCursor(QTextCursor::Start);
                mDisasTextEdit->setExtraSelections({});
                break;
            }

            cursor.movePosition(QTextCursor::EndOfLine);
            if (cursor.atEnd())
            {
                break;
            }

            cursor.movePosition(QTextCursor::Down);
        }

        // this is true if a seek came from the user clicking on a line.
        // then the cursor should be restored 1:1 to retain selection and cursor position.
        if (currentCursorOffset == offset)
        {
            mDisasTextEdit->setTextCursor(originalCursor);
        }
    }
    connectCursorPositionChanged(false);
}

void DisassemblyWidget::connectCursorPositionChanged(bool disconnect)
{
    if (disconnect)
    {
        QObject::disconnect(mDisasTextEdit, SIGNAL(cursorPositionChanged()), this, SLOT(cursorPositionChanged()));
    }
    else
    {
        connect(mDisasTextEdit, SIGNAL(cursorPositionChanged()), this, SLOT(cursorPositionChanged()));
    }
}

void DisassemblyWidget::cursorPositionChanged()
{
    RVA offset = readCurrentDisassemblyOffset();
    Core()->seek(offset);
    highlightCurrentLine();
    mCtxMenu->setCanCopy(mDisasTextEdit->textCursor().hasSelection());
}

bool DisassemblyWidget::eventFilter(QObject *obj, QEvent *event)
{
    if ((obj == mDisasTextEdit || obj == mDisasTextEdit->viewport()) && event->type() == QEvent::MouseButtonDblClick)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

        QTextCursor cursor = mDisasTextEdit->cursorForPosition(QPoint(mouseEvent->x(), mouseEvent->y()));
        RVA offset = readDisassemblyOffset(cursor);

        RVA jump = Core()->getOffsetJump(offset);

        if (jump == RVA_INVALID)
        {
            bool ok;
            RVA xref = Core()->cmdj("axfj@" + QString::number(offset)).array().first().toObject().value("to").toVariant().toULongLong(&ok);
            if (ok)
            {
                jump = xref;
            }
        }

        if (jump != RVA_INVALID)
        {
            CutterCore::getInstance()->seek(jump);
        }

        return true;
    }
    return QDockWidget::eventFilter(obj, event);
}

void DisassemblyWidget::on_seekChanged(RVA offset)
{
    if (topOffset != RVA_INVALID && bottomOffset != RVA_INVALID
        && offset >= topOffset && offset <= bottomOffset)
    {
        // if the line with the seek offset is currently visible, just move the cursor there
        updateCursorPosition();
    }
    else
    {
        // otherwise scroll there
        refreshDisasm(offset);
    }
    mCtxMenu->setOffset(offset);
}

void DisassemblyWidget::raisePrioritizedMemoryWidget(CutterCore::MemoryWidgetType type)
{
    if (type == CutterCore::MemoryWidgetType::Disassembly)
    {
        raise();
        setFocus();
    }
}

void DisassemblyWidget::fontsUpdatedSlot()
{
    setupFonts();

    if (!updateMaxLines()) // updateMaxLines() returns true if it already refreshed.
    {
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
    if (dy != 0)
    {
        emit scrollLines(dy);
    }

    if (event->type() == QEvent::Resize)
    {
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
    switch(event->type())
    {
        case QEvent::Type::Wheel:
            return false;
        default:
            return QAbstractScrollArea::viewportEvent(event);
    }
}

void DisassemblyTextEdit::scrollContentsBy(int dx, int dy)
{
    if (!lockScroll)
    {
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

    if (event->button() == Qt::RightButton && !textCursor().hasSelection())
    {
        setTextCursor(cursorForPosition(event->pos()));
    }
}

void DisassemblyWidget::seekPrev()
{
    Core()->seekPrev();
}
