#include "GraphDiffWidget.h"

#include <QHBoxLayout>
#include <QSplitter>
#include <QVBoxLayout>

GraphDiffWidget::GraphDiffWidget(CutterDiff *cutterDiff, CutterDiffWindow *parent)
    : CutterDiffWidget(cutterDiff, parent),
      leftView(new DiffGraphView(cutterDiff, this)),
      rightView(new DiffGraphView(cutterDiff, this))
{
    auto vBox = new QVBoxLayout(this);
    setLayout(vBox);

    // Top: current function name
    auto topHBox = new QHBoxLayout();
    functionLabel = new QLabel(this);
    functionLabel->setText("Current Function: ...");
    topHBox->addWidget(functionLabel);
    topHBox->addStretch();
    vBox->addLayout(topHBox, 0);

    // Middle: graph views
    auto splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(leftView);
    splitter->addWidget(rightView);
    vBox->addWidget(splitter, 1);

    // Bottom: graph mode selector
    auto bottomHBox = new QHBoxLayout();

    splitOrientationButton = new QPushButton(this);
    splitOrientationButton->setText("↕");
    splitOrientationButton->setToolTip("Toggle split view orientation");

    comboBox = new QComboBox(this);
    comboBox->addItem("Unified", UnifiedMode);
    comboBox->addItem("Split", SplitMode);
    comboBox->addItem("Original", OriginalMode);
    comboBox->addItem("Modified", ModifiedMode);

    bottomHBox->addStretch();
    bottomHBox->addWidget(splitOrientationButton);
    bottomHBox->addWidget(comboBox);

    vBox->addLayout(bottomHBox, 0);
    connect(cutterDiff, &CutterDiff::currentItemDiffChanged, this, &GraphDiffWidget::loadGraph);
    connect(comboBox, &QComboBox::currentIndexChanged, this, &GraphDiffWidget::loadGraph);
    connect(splitOrientationButton, &QPushButton::clicked, this, [this, splitter]() {
        changeSplitOrientation();
        splitter->setOrientation(graphSplitHorizontal ? Qt::Horizontal : Qt::Vertical);
    });
}

void GraphDiffWidget::changeSplitOrientation()
{
    graphSplitHorizontal = !graphSplitHorizontal;
    if (graphSplitHorizontal) {
        splitOrientationButton->setText("↕");
    } else {
        splitOrientationButton->setText("↔");
    }
}

void GraphDiffWidget::loadGraph()
{
    const CutterDiffItem &diffItem = cutterDiff->getCurrentDiffItem();
    const auto type = cutterDiff->getCurrentDiffItem().getType();

    if (!cutterDiff->getCurrentDiffItem().isFunction()) {
        return;
    }

    const auto graphMode =
            static_cast<DiffGraphMode>(comboBox->itemData(comboBox->currentIndex()).toInt());

    splitOrientationButton->setDisabled(true);
    comboBox->setDisabled(true);

    leftView->hide();
    rightView->hide();

    if (type == DiffItemMatched) {
        comboBox->setDisabled(false);
        functionLabel->setText(QString("%0->%1")
                                       .arg(diffItem.descriptionA()["name"].toString())
                                       .arg(diffItem.descriptionB()["name"].toString()));
        switch (graphMode) {
        case UnifiedMode: {
            leftView->show();
            leftView->loadCurrentGraph(Unified);
            break;
        }

        case SplitMode: {
            splitOrientationButton->setDisabled(false);
            leftView->show();
            rightView->show();
            leftView->loadCurrentGraph(Original);
            rightView->loadCurrentGraph(Modified);
            break;
        }

        case OriginalMode: {
            leftView->show();
            leftView->loadCurrentGraph(Original);
            break;
        }

        case ModifiedMode: {
            rightView->show();
            rightView->loadCurrentGraph(Modified);
            break;
        }
        default: {
            break;
        }
        }

    } else if (type == DiffItemRemoved) {
        qInfo() << "yes";
        const QSignalBlocker blocker(comboBox);
        comboBox->setCurrentIndex(OriginalMode);
        leftView->loadCurrentGraph(Original);
        leftView->show();
    } else if (type == DiffItemAdded) {
        const QSignalBlocker blocker(comboBox);
        comboBox->setCurrentIndex(ModifiedMode);
        rightView->loadCurrentGraph(Modified);
        rightView->show();
    }
    leftView->refreshView();
    rightView->refreshView();
}
