#include "LineDiffWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QScrollBar>
#include <QSplitter>
#include <QTextBlock>
#include <QVBoxLayout>

#include <Configuration.h>

LineDiffWidget::LineDiffWidget(CutterDiff *cutterDiff, QWidget *parent)
    : QWidget(parent),
      cutterDiff(cutterDiff),
      leftEdit(new DiffTextEdit(this)),
      rightEdit(new DiffTextEdit(this)),
      unifiedEdit(new DiffTextEdit(this)),
      viewSelector(new QComboBox(this)),
      splitViewSplitter(new QSplitter(Qt::Horizontal, this))
{
    auto *layoutV = new QVBoxLayout(this);
    layoutV->setContentsMargins(0, 0, 0, 0);
    layoutV->setSpacing(0);
    auto *layoutHEdits = new QHBoxLayout(this);
    layoutV->addLayout(layoutHEdits);
    layoutHEdits->addWidget(unifiedEdit);

    splitViewSplitter->addWidget(leftEdit);
    splitViewSplitter->addWidget(rightEdit);
    layoutHEdits->addWidget(splitViewSplitter);
    auto layoutH = new QHBoxLayout(this);
    layoutH->addStretch();
    auto labelLineDiff = new QLabel(this);
    labelLineDiff->setText("View : ");
    layoutH->addWidget(labelLineDiff);
    viewSelector->addItems({ "Unified", "Split", "Left", "Right" });
    connect(viewSelector, &QComboBox::currentIndexChanged, this,
            &LineDiffWidget::onViewModeChanged);

    layoutH->addWidget(viewSelector);
    layoutV->addLayout(layoutH);

    connect(leftEdit->verticalScrollBar(), &QScrollBar::valueChanged, this,
            [this](int value) { rightEdit->verticalScrollBar()->setValue(value); });

    connect(rightEdit->verticalScrollBar(), &QScrollBar::valueChanged, this,
            [this](int value) { leftEdit->verticalScrollBar()->setValue(value); });
    setUpFonts();
    connect(Config(), &Configuration::fontsUpdated, this, &LineDiffWidget::setUpFonts);
    onViewModeChanged();
}

LineDiffWidget::~LineDiffWidget() {}

void LineDiffWidget::onViewModeChanged()
{
    splitViewSplitter->hide();
    unifiedEdit->hide();
    leftEdit->hide();
    rightEdit->hide();
    switch (viewSelector->currentIndex()) {
    case 0:
        unifiedEdit->show();
        break;
    case 1: {
        splitViewSplitter->show();
        leftEdit->show();
        rightEdit->show();
        break;
    }
    case 2: {
        splitViewSplitter->show();
        leftEdit->show();
        rightEdit->hide();
        break;
    }
    case 3: {
        splitViewSplitter->show();
        leftEdit->hide();
        rightEdit->show();
        break;
    }
    default:
        break;
    }
}

void LineDiffWidget::setUpFonts()
{
    leftEdit->setUpFont(Config()->getFont());
    rightEdit->setUpFont(Config()->getFont());
    unifiedEdit->setUpFont(Config()->getFont());
}

void LineDiffWidget::fetchFunctionDisasSplit(RVA addrA, RVA addrB)
{
    const QSignalBlocker blocker1(leftEdit), blocker2(rightEdit), blocker3(unifiedEdit);
    leftEdit->clear();
    rightEdit->clear();
    char *stringUtf;
    RzDiff *diffedLines = cutterDiff->diffFunctionDissas(addrA, addrB);
    QColor matched = Config()->getColor("gui.match.perfect");
    QColor unmatched = Config()->getColor("gui.match.partial");
    matched.setAlpha(50);
    unmatched.setAlpha(50);
    const CutterRzList<RzList /*<RzDiffOp *>*/> groups =
            cutterDiff->lineDiffOpsGrouped(diffedLines);
    for (const RzList /*<RzDiffOp*>*/ *group : groups) {
        for (RzDiffOp *op : CutterRzList<RzDiffOp>(group)) {

            switch (op->type) {
            case RZ_DIFF_OP_EQUAL: {
                stringUtf = rz_diff_op_stringify(diffedLines, op, true);
                const QString opString = QString::fromUtf8(stringUtf);
                leftEdit->insertFormatted(opString, { 0, 0, 0, 0 });
                rightEdit->insertFormatted(opString, { 0, 0, 0, 0 });
                unifiedEdit->insertFormatted(opString, { 0, 0, 0, 0 });
                break;
            }
            case RZ_DIFF_OP_DELETE: {
                stringUtf = rz_diff_op_stringify(diffedLines, op, true);
                leftEdit->insertFormatted(QString::fromUtf8(stringUtf), unmatched);
                unifiedEdit->insertFormatted(QString::fromUtf8(stringUtf), unmatched);
                break;
            }
            case RZ_DIFF_OP_INSERT: {
                stringUtf = rz_diff_op_stringify(diffedLines, op, false);
                rightEdit->insertFormatted(QString::fromUtf8(stringUtf), matched);
                unifiedEdit->insertFormatted(QString::fromUtf8(stringUtf), matched);
                break;
            }
            case RZ_DIFF_OP_REPLACE: {
                const QString actual =
                        QString::fromUtf8(rz_diff_op_stringify(diffedLines, op, true));
                const QString replaced =
                        QString::fromUtf8(rz_diff_op_stringify(diffedLines, op, false));
                const auto bound = cutterDiff->getLineDiffBounds(actual, replaced);
                leftEdit->insertBounded(actual, unmatched, bound);
                rightEdit->insertBounded(replaced, matched, bound);
                unifiedEdit->insertBounded(actual, unmatched, bound);
                unifiedEdit->insertBounded(replaced, matched, bound);
                break;
            }
            default:
                break;
            }
            balanceLines();
        }
    }
    rz_diff_free(diffedLines);
}

void LineDiffWidget::balanceLines()
{
    const int leftLines = leftEdit->document()->blockCount();
    const int rightLines = rightEdit->document()->blockCount();

    if (leftLines == rightLines) {
        return;
    }

    DiffTextEdit *edit = (leftLines < rightLines) ? leftEdit : rightEdit;
    const int missing = std::abs(leftLines - rightLines);

    QTextCursor cursor(edit->document());
    cursor.movePosition(QTextCursor::End);

    for (int i = 0; i < missing; ++i) {
        cursor.insertBlock();
    }

    edit->setTextCursor(cursor);
}

DiffTextEdit::DiffTextEdit(QWidget *parent) : QPlainTextEdit(parent)
{
    setReadOnly(true);
    setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);
    lineNumberArea = new LineNumberArea(this);
    connect(this, &QPlainTextEdit::blockCountChanged, this, &DiffTextEdit::updateLineNumberArea);

    connect(verticalScrollBar(), &QScrollBar::valueChanged, this,
            &DiffTextEdit::updateLineNumberArea);

    connect(this, &QPlainTextEdit::cursorPositionChanged, this,
            &DiffTextEdit::highlightCurrentLine);
    setLineWrapMode(QPlainTextEdit::NoWrap);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

DiffTextEdit::~DiffTextEdit() {}

void DiffTextEdit::highlightCurrentLine()
{
    const QTextBlock currentBlock = textCursor().block();

    if (highlightedBlock.isValid() && highlightedBlock != currentBlock) {
        QTextCursor oldCursor(highlightedBlock);
        QTextBlockFormat oldFmt = highlightedBlock.blockFormat();
        oldFmt.clearBackground();
        oldCursor.setBlockFormat(oldFmt);
    }

    QTextCursor cursor(currentBlock);
    QTextBlockFormat fmt = currentBlock.blockFormat();
    fmt.setBackground(Config()->getColor("gui.background").lighter(100));
    cursor.setBlockFormat(fmt);

    highlightedBlock = currentBlock;
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

    painter.fillRect(event->rect(), Config()->getColor("gui.background").darker(115));

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

    const QTextCharFormat oldFormat = defaultFormat;

    QTextCharFormat fmt = oldFormat;
    fmt.setBackground(color);

    cursor.insertText(text, fmt);

    cursor.setCharFormat(oldFormat);
    setTextCursor(cursor);
}

void DiffTextEdit::insertBounded(const QString &text, const QColor &color, const Bound bound)
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    const QTextCharFormat oldFormat = defaultFormat;
    const QColor colorHighlight = { color.red(), color.green(), color.blue(), 255 };
    QTextCharFormat fmt = oldFormat;
    fmt.setBackground(color);

    cursor.insertText(text.left(bound.pos), fmt);
    fmt.setBackground(colorHighlight);
    cursor.insertText(text.mid(bound.pos, bound.size), fmt);
    fmt.setBackground(color);
    cursor.insertText(text.mid(bound.pos + bound.size), fmt);
    cursor.setCharFormat(oldFormat);
    setTextCursor(cursor);
}

void DiffTextEdit::setUpFont(const QFont &font)
{
    setFont(font);
    lineNumberArea->setFont(font);
}