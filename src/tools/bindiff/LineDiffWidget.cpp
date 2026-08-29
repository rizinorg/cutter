#include "LineDiffWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QScrollBar>
#include <QSplitter>
#include <QTextBlock>
#include <QVBoxLayout>

#include <Configuration.h>

LineDiffWidget::LineDiffWidget(CutterDiff *cutterDiff, CutterDiffWindow *parent)
    : CutterDiffWidget(cutterDiff, parent),
      leftEdit(new DiffTextEdit(this)),
      rightEdit(new DiffTextEdit(this)),
      unifiedEdit(new DiffTextEdit(this)),
      viewSelector(new QComboBox(this)),
      splitViewSplitter(new QSplitter(Qt::Horizontal, this))
{
    auto *layoutV = new QVBoxLayout(this);
    layoutV->setContentsMargins(0, 0, 0, 0);
    layoutV->setSpacing(2);

    // Header
    auto *layoutHHeader = new QHBoxLayout();

    functionLabel = new QLabel(this);
    layoutHHeader->addWidget(functionLabel);

    layoutV->addLayout(layoutHHeader, 0);

    // Editors
    auto *layoutHEdits = new QHBoxLayout();

    layoutHEdits->addWidget(unifiedEdit);

    splitViewSplitter->addWidget(leftEdit);
    splitViewSplitter->addWidget(rightEdit);

    layoutHEdits->addWidget(splitViewSplitter);

    layoutV->addLayout(layoutHEdits, 1);

    // Bottom controls
    auto *layoutH = new QHBoxLayout();

    layoutH->addStretch();

    auto *labelLineDiff = new QLabel("View : ", this);
    layoutH->addWidget(labelLineDiff);

    splitOrientationButton = new QPushButton(this);
    splitOrientationButton->setText("↕");
    splitOrientationButton->setToolTip("Toggle split view orientation");

    layoutH->addWidget(splitOrientationButton);

    layoutV->addLayout(layoutH, 0);

    viewSelector->addItems({ "Unified", "Split", "Left", "Right" });
    connect(viewSelector, &QComboBox::currentIndexChanged, this,
            &LineDiffWidget::onViewModeChanged);

    layoutH->addWidget(viewSelector);
    layoutV->addLayout(layoutH, 0);

    connect(splitOrientationButton, &QPushButton::clicked, this, [this]() {
        splitHorizontal = !splitHorizontal;
        onViewModeChanged();
    });

    // Implement Syncronouse Scrolling compatible with Horizontal view as well

    // connect(leftEdit->verticalScrollBar(), &QScrollBar::valueChanged, this,
    //         [this](int value) { rightEdit->verticalScrollBar()->setValue(value); });

    // connect(rightEdit->verticalScrollBar(), &QScrollBar::valueChanged, this,
    //         [this](int value) { leftEdit->verticalScrollBar()->setValue(value); });
    setUpFonts();
    connect(Config(), &Configuration::fontsUpdated, this, &LineDiffWidget::setUpFonts);
    onViewModeChanged();

    connect(cutterDiff, &CutterDiff::currentItemDiffChanged, this,
            [this]() { fetchFunctionDisasSplit(this->cutterDiff->getCurrentDiffItem()); });
}

LineDiffWidget::~LineDiffWidget() {}

void LineDiffWidget::onViewModeChanged()
{
    splitViewSplitter->hide();
    unifiedEdit->hide();
    leftEdit->hide();
    rightEdit->hide();
    if (splitHorizontal) {
        splitOrientationButton->setText("↕");
    } else {
        splitOrientationButton->setText("↔");
    }
    splitOrientationButton->setDisabled(true);
    switch (viewSelector->currentIndex()) {
    case 0:
        unifiedEdit->show();
        break;
    case 1: {
        splitViewSplitter->show();
        leftEdit->show();
        rightEdit->show();
        splitOrientationButton->setDisabled(false);
        splitViewSplitter->setOrientation(splitHorizontal ? Qt::Horizontal : Qt::Vertical);
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

void LineDiffWidget::fetchFunctionDisasSplit(const CutterDiffItem &diffItem)
{
    if (!diffItem.isFunction()) {
        // Unable to load function may be a message
        return;
    }
    QColor matched = Config()->getColor("gui.match.perfect");
    QColor unmatched = Config()->getColor("gui.match.partial");
    viewSelector->setDisabled(true);
    matched.setAlpha(50);
    unmatched.setAlpha(50);
    if (!diffItem.isFunction()) {
        return;
    }
    leftEdit->clear();
    rightEdit->clear();
    unifiedEdit->clear();
    if (diffItem.getType() == DiffItemMatched) {
        functionLabel->setText(QString("%0 -> %1")
                                       .arg(diffItem.descriptionA()["name"].toString())
                                       .arg(diffItem.descriptionB()["name"].toString()));
        viewSelector->setDisabled(false);
        const QSignalBlocker blocker1(leftEdit), blocker2(rightEdit), blocker3(unifiedEdit);
        if (!diffItem.getInstrDiffs().contains("disas")) {
            // TODO: show disas not available in the window
            return;
        }
        for (const DiffInstr &instr : diffItem.getInstrDiffs()["disas"]) {

            switch (instr.type) {
            case DiffInstrEqual: {
                leftEdit->insertFormatted(instr.a, { 0, 0, 0, 0 });
                rightEdit->insertFormatted(instr.a, { 0, 0, 0, 0 });
                unifiedEdit->insertFormatted(instr.a, { 0, 0, 0, 0 });
                break;
            }
            case DiffInstrDeleted: {
                leftEdit->insertFormatted(instr.a, unmatched);
                unifiedEdit->insertFormatted(instr.a, unmatched);
                break;
            }
            case DiffInstrInserted: {
                rightEdit->insertFormatted(instr.b, matched);
                unifiedEdit->insertFormatted(instr.b, matched);
                break;
            }
            case DiffInstrReplaced: {
                leftEdit->insertBounded(instr.a, unmatched, instr.bound);
                rightEdit->insertBounded(instr.b, matched, instr.bound);
                unifiedEdit->insertBounded(instr.a, unmatched, instr.bound);
                unifiedEdit->insertBounded(instr.b, matched, instr.bound);
                break;
            }
            default:
                break;
            }
            balanceLines();
        }
    } else if (diffItem.getType() == DiffItemRemoved) {
        functionLabel->setText(QString("%0").arg(diffItem.descriptionA()["name"].toString()));
        if (diffItem.descriptionA().contains("disas")) {
            unifiedEdit->insertFormatted(diffItem.descriptionA()["disas"].toString(), unmatched);
            viewSelector->setCurrentIndex(0);
            return;
        }
    } else if (diffItem.getType() == DiffItemAdded) {
        functionLabel->setText(QString("%0").arg(diffItem.descriptionB()["name"].toString()));
        if (!diffItem.descriptionB().contains("disas")) {
            unifiedEdit->insertFormatted(diffItem.descriptionB()["disas"].toString(), matched);
            viewSelector->setCurrentIndex(0);
            return;
        }
    }
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

    if (!currentBlock.isValid()) {
        return;
    }

    if (highlightedBlock.isValid() && highlightedBlock != currentBlock) {
        QTextCursor cursor(highlightedBlock);
        QTextBlockFormat format = highlightedBlock.blockFormat();
        format.clearBackground();
        cursor.setBlockFormat(format);
    }

    QTextCursor cursor(currentBlock);
    QTextBlockFormat format = currentBlock.blockFormat();

    QColor color = Config()->getColor("gui.background");
    color.setAlpha(40);

    format.setBackground(color);
    cursor.setBlockFormat(format);

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