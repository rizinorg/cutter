#include "GraphDiffWidget.h"

#include <QHBoxLayout>
#include <QSplitter>
#include <QVBoxLayout>

GraphDiffWidget::GraphDiffWidget(CutterDiff *cutterDiff, QWidget *parent)
    : cutterDiff(cutterDiff),
      leftView(new DiffGraphView(cutterDiff, this)),
      rightView(new DiffGraphView(cutterDiff, this)),
      QWidget { parent }
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
    const auto graphMode =
            static_cast<DiffGraphMode>(comboBox->itemData(comboBox->currentIndex()).toInt());

    if (!cutterDiff->getCurrentDiffItem().isFunction()) {
        return;
    }

    if (type != DiffItemMatched) {
        if (type == DiffItemRemoved) {
            comboBox->setCurrentIndex(OriginalMode);
        } else {
            comboBox->setCurrentIndex(ModifiedMode);
        }
        comboBox->setDisabled(true);
    } else {
        comboBox->setDisabled(false);
    }

    splitOrientationButton->setDisabled(true);

    switch (type) {
    case DiffItemMatched: {
        functionLabel->setText(QString("%0->%1")
                                       .arg(diffItem.descriptionA()["name"].toString())
                                       .arg(diffItem.descriptionB()["name"].toString()));
        switch (graphMode) {
        case UnifiedMode:
            leftView->show();
            rightView->hide();
            leftView->loadCurrentGraph(Unified);
            break;

        case SplitMode:
            splitOrientationButton->setDisabled(false);
            leftView->show();
            rightView->show();
            leftView->loadCurrentGraph(Original);
            rightView->loadCurrentGraph(Modified);
            break;

        case OriginalMode:
            leftView->show();
            rightView->hide();
            leftView->loadCurrentGraph(Original);
            break;

        case ModifiedMode:
            leftView->hide();
            rightView->show();
            rightView->loadCurrentGraph(Modified);
            break;
        }
        break;
    }

    case DiffItemRemoved:
        functionLabel->setText(QString("%0").arg(diffItem.descriptionA()["name"].toString()));
        switch (graphMode) {
        case OriginalMode:
            leftView->show();
            rightView->hide();
            leftView->loadCurrentGraph(Original);
            break;

        default:
            leftView->hide();
            rightView->show();
            rightView->loadCurrentGraph(Modified);
            break;
        }
        break;

    case DiffItemAdded:
        functionLabel->setText(QString("%0").arg(diffItem.descriptionB()["name"].toString()));
        switch (graphMode) {
        case ModifiedMode:
            leftView->hide();
            rightView->show();
            rightView->loadCurrentGraph(Modified);
            break;

        default:
            leftView->show();
            rightView->hide();
            leftView->loadCurrentGraph(Original);
            break;
        }
        break;

    default:
        functionLabel->setText("unknown");
        leftView->hide();
        rightView->hide();
        break;
    }
}
