#include "DisassemblyWidget.h"
#include "menus/DisassemblyContextMenu.h"
#include "dialogs/XrefsDialog.h"
#include "utils/HexAsciiHighlighter.h"
#include "utils/HexHighlighter.h"
#include "utils/Configuration.h"

#include <QScrollBar>
#include <QJsonArray>
#include <QJsonObject>
#include <QVBoxLayout>


DisassemblyWidget::DisassemblyWidget(QWidget *parent) :
    QDockWidget(parent),
    mDisasScrollArea(new DisassemblyScrollArea(this)),
    mDisasTextEdit(new DisassemblyTextEdit(this))
{
    // Configure Dock

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

    // Increase asm text edit margin
    QTextDocument *asm_docu = mDisasTextEdit->document();
    asm_docu->setDocumentMargin(10);


    // Event filter to intercept double clicks in the textbox
    mDisasTextEdit->viewport()->installEventFilter(this);

    // Set Disas context menu
    mDisasTextEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(mDisasTextEdit, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showDisasContextMenu(const QPoint &)));

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

QString DisassemblyWidget::readDisasm(RVA offset, bool backwards, bool skipFirstInstruction)
{
    QString cmd;
    if (backwards)
    {
        // Strategy for getting correct backwards disassembly in the most cases:

        // Disassemble a couple instructions at an offset before the current.
        // The last of these instructions is most likely a correct one. Use this as a reference.
        QJsonValue referenceInstValue = Core()->cmdj("pdj 10 @ " + QString::number(offset - 256)).array().last();
        // TODO: handle offset < 256 and referenceInstValue.isNull()
        QJsonObject referenceInst = referenceInstValue.toObject();
        RVA referenceOffset = referenceInst["offset"].toVariant().toULongLong();
        // TODO: handle referenceOffset >= offset

        // Then just disassemble all bytes from the reference offset to the current offset.
        cmd = "pD " + QString::number(offset - referenceOffset) + "@" + QString::number(referenceOffset);
    }
    else
    {
        QString suffix = "@" + QString::number(offset);

        // skip size of first instruction if needed
        if (skipFirstInstruction && !backwards)
        {
            QJsonArray array = Core()->cmdj("pdj 1" + suffix).array();
            if (!array.isEmpty())
            {
                int instSize = array.first().toObject()["size"].toInt();
                suffix = "@" + QString::number(offset + instSize);
            }
        }

        cmd = "pd 100" + suffix;
    }

    return readDisasm(cmd, !backwards);
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

    QString disas = readDisasm("pd " + QString::number(maxLines) + "@" + QString::number(topOffset), true);
    connectCursorPositionChanged(true);
    mDisasTextEdit->document()->setHtml(disas);
    mDisasTextEdit->moveCursor(QTextCursor::End);
    connectCursorPositionChanged(false);
    bottomOffset = readCurrentDisassemblyOffset();
    printf("bottom: %#llx\n", bottomOffset);
    updateCursorPosition();
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
        QJsonArray array = Core()->cmdj("pdj " + QString::number(count) + "@" + QString::number(topOffset)).array();
        if (array.isEmpty())
        {
            return;
        }

        QJsonValue instValue = (count < 0 ? array.first() : array.last());
        if (!instValue.isObject())
        {
            return;
        }

        bool ok;
        offset = instValue.toObject()["offset"].toVariant().toULongLong(&ok);
        if (!ok)
        {
            return;
        }
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
}

void DisassemblyWidget::showDisasContextMenu(const QPoint &pt)
{
    DisassemblyContextMenu menu(this->readCurrentDisassemblyOffset(), mDisasTextEdit);
    menu.exec(mDisasTextEdit->mapToGlobal(pt));
}

RVA DisassemblyWidget::readCurrentDisassemblyOffset()
{
    // TODO: do this in a different way without parsing the disassembly text
    QTextCursor tc = mDisasTextEdit->textCursor();
    tc.select(QTextCursor::LineUnderCursor);
    QString lastline = tc.selectedText();
    QStringList parts = lastline.split("\u00a0", QString::SkipEmptyParts);

    if (parts.isEmpty()) {
        return RVA_INVALID;
    }

    QString ele = parts[0];
    if (!ele.contains("0x")) {
        return RVA_INVALID;
    }

    return ele.toULongLong(0, 16);
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
        QTextCursor cursor = mDisasTextEdit->textCursor();
        cursor.movePosition(QTextCursor::Start);

        while (!cursor.atEnd())
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
            cursor.movePosition(QTextCursor::Down);
            cursor.movePosition(QTextCursor::EndOfLine);
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
    refreshDisasm(offset);
}

void DisassemblyWidget::fontsUpdatedSlot()
{
    mDisasTextEdit->setFont(Config()->getFont());
    refreshDisasm();
}

void DisassemblyWidget::showXrefsDialog()
{
    // Get current offset
    QTextCursor tc = mDisasTextEdit->textCursor();
    tc.select(QTextCursor::LineUnderCursor);
    QString lastline = tc.selectedText();
    QString ele = lastline.split(" ", QString::SkipEmptyParts)[0];
    if (ele.contains("0x"))
    {
        RVA addr = ele.toLongLong(0, 16);
        XrefsDialog *dialog = new XrefsDialog(this);
        dialog->fillRefsForAddress(addr, RAddressString(addr), false);
        dialog->exec();
    }
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