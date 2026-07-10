#include "LineDiffWidget.h"

#include <QPainter>
#include <QScrollBar>
#include <QSplitter>
#include <QTextBlock>
#include <QVBoxLayout>

LineDiffWidget::LineDiffWidget(CutterDiff *cutterDiff, QWidget *parent)
    : QWidget(parent),
      cutterDiff(cutterDiff),
      leftEdit(new DiffTextEdit(this)),
      rightEdit(new DiffTextEdit(this))
{
    auto *layoutV = new QVBoxLayout(this);
    layoutV->setContentsMargins(0, 0, 0, 0);
    layoutV->setSpacing(0);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(leftEdit);
    splitter->addWidget(rightEdit);

    layoutV->addWidget(splitter);
}

LineDiffWidget::~LineDiffWidget() {}

void LineDiffWidget::fetchFunctionDisas(RVA addrA, RVA addrB)
{
    char *stringUtf;
    RzDiff *diffedLines = cutterDiff->diffFunctionDissas(addrA, addrB);
    const CutterRzList<RzList /*<RzDiffOp *>*/> groups =
            cutterDiff->lineDiffOpsGrouped(diffedLines);
    for (const RzList /*<RzDiffOp*>*/ *group : groups) {
        for (RzDiffOp *op : CutterRzList<RzDiffOp>(group)) {
            switch (op->type) {
            case RZ_DIFF_OP_EQUAL: {
                stringUtf = rz_diff_op_stringify(diffedLines, op, true);
                const QString opString = QString::fromUtf8(stringUtf);
                leftEdit->insertPlainText(opString);
                rightEdit->insertPlainText(opString);
                break;
            }
            case RZ_DIFF_OP_DELETE: {
                stringUtf = rz_diff_op_stringify(diffedLines, op, true);
                leftEdit->insertFormatted(QString::fromUtf8(stringUtf), QColor(255, 0, 0, 100));
                break;
            }
            case RZ_DIFF_OP_INSERT: {
                stringUtf = rz_diff_op_stringify(diffedLines, op, false);
                rightEdit->insertFormatted(QString::fromUtf8(stringUtf), QColor(0, 255, 0, 100));
                break;
            }
            case RZ_DIFF_OP_REPLACE: {
                stringUtf = rz_diff_op_stringify(diffedLines, op, true);
                leftEdit->insertFormatted(QString::fromUtf8(stringUtf), QColor(255, 0, 0, 100));
                stringUtf = rz_diff_op_stringify(diffedLines, op, false);
                rightEdit->insertFormatted(QString::fromUtf8(stringUtf), QColor(0, 255, 0, 100));
                break;
            }
            default:
                break;
            }
        }
    }
    rz_diff_free(diffedLines);
}

DiffTextEdit::DiffTextEdit(QWidget *parent) : QPlainTextEdit(parent)
{
    setReadOnly(true);
    lineNumberArea = new LineNumberArea(this);
    connect(this, &QPlainTextEdit::blockCountChanged, this, &DiffTextEdit::updateLineNumberArea);

    connect(verticalScrollBar(), &QScrollBar::valueChanged, this,
            &DiffTextEdit::updateLineNumberArea);

    connect(this, &QPlainTextEdit::cursorPositionChanged, this,
            &DiffTextEdit::highlightCurrentLine);
}

DiffTextEdit::~DiffTextEdit() {}

void DiffTextEdit::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> selections;

    QTextEdit::ExtraSelection selection;

    QColor lineColor = QColor(60, 60, 60); // Choose a theme-appropriate color
    selection.format.setBackground(lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);

    selection.cursor = textCursor();
    selection.cursor.clearSelection();

    selections.append(selection);

    setExtraSelections(selections);
}

void DiffTextEdit::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);

    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);

    lineNumberArea->setGeometry(0, 0, lineNumberAreaWidth(), height());
}

int DiffTextEdit::lineNumberAreaWidth() const
{
    const int digits = QString::number(document()->blockCount()).length();

    return 8 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void DiffTextEdit::updateLineNumberArea()
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
    lineNumberArea->update();
}

void DiffTextEdit::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);

    painter.fillRect(event->rect(), QColor(240, 240, 240));

    QTextBlock block = firstVisibleBlock();

    while (block.isValid()) {
        const int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
        const int bottom = top + qRound(blockBoundingRect(block).height());
        ;

        if (bottom >= event->rect().top() && top <= event->rect().bottom()) {
            painter.setPen(Qt::darkGray);

            painter.drawText(0, top, lineNumberArea->width() - 4, fontMetrics().height(),
                             Qt::AlignRight, QString::number(block.blockNumber() + 1));
        }
        block = block.next();
    }
}

void DiffTextEdit::insertFormatted(const QString &text, const QColor &color)
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);

    const QTextCharFormat oldFormat = cursor.charFormat();

    QTextCharFormat fmt = oldFormat;
    fmt.setBackground(color);

    cursor.insertText(text, fmt);

    cursor.setCharFormat(oldFormat);
    setTextCursor(cursor);
}