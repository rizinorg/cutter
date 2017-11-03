#include "DisassemblyWidget.h"
#include "menus/DisassemblyContextMenu.h"
#include "utils/HexAsciiHighlighter.h"
#include "utils/HexHighlighter.h"
#include "utils/Configuration.h"

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

    // Scrollbar
    connect(mDisasTextEdit->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(disasmScrolled()));
    // x to show XRefs
    QShortcut *shortcut_x = new QShortcut(QKeySequence(Qt::Key_X), mDisasTextEdit);
    shortcut_x->setContext(Qt::WidgetShortcut);
    connect(shortcut_x, SIGNAL(activated()), this, SLOT(showXrefsDialog()));


    maxLines = 0;
    updateMaxLines();


    connect(mDisasScrollArea, SIGNAL(scrollLines(int)), this, SLOT(scrollInstructions(int)));
    connect(mDisasScrollArea, SIGNAL(disassemblyResized()), this, SLOT(updateMaxLines()));

    connectCursorPositionChanged(false);
    connect(mDisasTextEdit->verticalScrollBar(), &QScrollBar::valueChanged, this, [=](int value) {
        if (value != 0)
        {
            mDisasTextEdit->verticalScrollBar()->setValue(0);
        }
    });

    // Seek signal
    connect(CutterCore::getInstance(), SIGNAL(seekChanged(RVA)), this, SLOT(on_seekChanged(RVA)));
    connect(CutterCore::getInstance(), SIGNAL(commentsChanged()), this, SLOT(refreshDisasm()));
    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(fontsUpdatedSlot()));
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
    Core()->setConfig("scr.html", true);
    Core()->setConfig("scr.color", true);
    QString disas = Core()->cmd(cmd);
    Core()->setConfig("scr.html", false);
    Core()->setConfig("scr.color", false);

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
        mDisasTextEdit->clear();
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
    tc.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, maxLines-1);
    mDisasTextEdit->setTextCursor(tc);

    connectCursorPositionChanged(false);

    bottomOffset = readCurrentDisassemblyOffset();
    if (bottomOffset == RVA_INVALID)
    {
        bottomOffset = topOffset;
    }

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


void DisassemblyWidget::updateMaxLines()
{
    QFontMetrics fontMetrics(mDisasTextEdit->document()->defaultFont());
    int currentMaxLines = (mDisasTextEdit->height() -
                           (mDisasTextEdit->contentsMargins().top() + mDisasTextEdit->contentsMargins().bottom()
                            + (int)(mDisasTextEdit->document()->documentMargin() * 2)))
                          / fontMetrics.lineSpacing();

    if (currentMaxLines != maxLines)
    {
        maxLines = currentMaxLines;
        refreshDisasm();
    }
}

void DisassemblyWidget::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    // Highlight the current line in yellow
    if (mDisasTextEdit->isReadOnly())
    {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(190, 144, 212);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = mDisasTextEdit->textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    // Highlight the current word
    QTextCursor cursor = mDisasTextEdit->textCursor();
    cursor.select(QTextCursor::WordUnderCursor);

    QTextEdit::ExtraSelection currentWord;

    QColor blueColor = QColor(Qt::blue).lighter(160);
    currentWord.format.setBackground(blueColor);

    currentWord.cursor = cursor;
    extraSelections.append(currentWord);
    currentWord.cursor.clearSelection();

    // Highlight all the words in the document same as the actual one
    QString searchString = cursor.selectedText();
    QTextDocument *document = mDisasTextEdit->document();

    //QTextCursor highlightCursor(document);
    QTextEdit::ExtraSelection highlightSelection;
    highlightSelection.cursor = cursor;
    highlightSelection.format.setBackground(blueColor);
    QTextCursor cursor2(document);

    cursor2.beginEditBlock();

    highlightSelection.cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
    while (!highlightSelection.cursor.isNull() && !highlightSelection.cursor.atEnd())
    {
        highlightSelection.cursor = document->find(searchString, highlightSelection.cursor, QTextDocument::FindWholeWords);

        if (!highlightSelection.cursor.isNull())
        {
            highlightSelection.cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
            extraSelections.append(highlightSelection);
        }
    }
    cursor2.endEditBlock();

    mDisasTextEdit->setExtraSelections(extraSelections);
    mCtxMenu->setOffset(readCurrentDisassemblyOffset());
}

void DisassemblyWidget::showDisasContextMenu(const QPoint &pt)
{
    mCtxMenu->exec(mDisasTextEdit->mapToGlobal(pt));
}

RVA DisassemblyWidget::readCurrentDisassemblyOffset()
{
    // TODO: do this in a different way without parsing the disassembly text

    static const QRegularExpression offsetRegExp("^0x[0-9A-Fa-f]*");

    QTextCursor tc = mDisasTextEdit->textCursor();

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
    connectCursorPositionChanged(true);
    RVA offset = Core()->getOffset();

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
            mDisasTextEdit->setTextCursor(cursor);
            RVA lineOffset = readCurrentDisassemblyOffset();
            if (lineOffset == offset)
            {
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
}


bool DisassemblyWidget::eventFilter(QObject *obj, QEvent *event)
{
    if ((obj == mDisasTextEdit || obj == mDisasTextEdit->viewport()) && event->type() == QEvent::MouseButtonDblClick)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        //qDebug()<<QString("Click location: (%1,%2)").arg(mouseEvent->x()).arg(mouseEvent->y());
        QTextCursor cursor = mDisasTextEdit->cursorForPosition(QPoint(mouseEvent->x(), mouseEvent->y()));
        cursor.select(QTextCursor::LineUnderCursor);
        QString lastline = cursor.selectedText();
        auto eles = lastline.split(" ", QString::SkipEmptyParts);
        QString ele = eles.isEmpty() ? "" : eles[0];
        if (ele.contains("0x"))
        {
            QString jump = CutterCore::getInstance()->getOffsetJump(ele);
            if (!jump.isEmpty())
            {
                if (jump.contains("0x"))
                {
                    QString fcn = CutterCore::getInstance()->cmdFunctionAt(jump);
                    if (!fcn.isEmpty())
                    {
                        RVA addr = jump.trimmed().toULongLong(0, 16);
                        CutterCore::getInstance()->seek(addr);
                    }
                }
                else
                {
                    RVA addr = CutterCore::getInstance()->cmd("?v " + jump).toULongLong(0, 16);
                    CutterCore::getInstance()->seek(addr);
                }
            }
        }
    }
    return QDockWidget::eventFilter(obj, event);
}

void DisassemblyWidget::on_seekChanged(RVA offset)
{
    Q_UNUSED(offset);
    if (!Core()->graphDisplay || !Core()->graphPriority) {
        this->raise();
    }

    mCtxMenu->setOffset(offset);

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
}

void DisassemblyWidget::fontsUpdatedSlot()
{
    mDisasTextEdit->setFont(Config()->getFont());
    refreshDisasm();
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
