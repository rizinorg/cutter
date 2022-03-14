#include "BacktraceWidget.h"
#include "ui_BacktraceWidget.h"
#include "common/JsonModel.h"
#include "QHeaderView"

#include "core/MainWindow.h"

BacktraceWidget::BacktraceWidget(MainWindow *main)
    : CutterDockWidget(main), ui(new Ui::BacktraceWidget)
{
    ui->setupUi(this);

    // setup backtrace model
    QString PC = Core()->getRegisterName("PC");
    QString SP = Core()->getRegisterName("SP");
    modelBacktrace->setHorizontalHeaderItem(0, new QStandardItem(tr("Function")));
    modelBacktrace->setHorizontalHeaderItem(1, new QStandardItem(SP));
    modelBacktrace->setHorizontalHeaderItem(2, new QStandardItem(PC));
    modelBacktrace->setHorizontalHeaderItem(3, new QStandardItem(tr("Description")));
    modelBacktrace->setHorizontalHeaderItem(4, new QStandardItem(tr("Frame Size")));
    viewBacktrace->setFont(Config()->getFont());
    viewBacktrace->setModel(modelBacktrace);
    viewBacktrace->verticalHeader()->setVisible(false);
    viewBacktrace->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->verticalLayout->addWidget(viewBacktrace);

    refreshDeferrer = createRefreshDeferrer([this]() { updateContents(); });

    connect(Core(), &CutterCore::refreshAll, this, &BacktraceWidget::updateContents);
    connect(Core(), &CutterCore::registersChanged, this, &BacktraceWidget::updateContents);
    connect(Config(), &Configuration::fontsUpdated, this, &BacktraceWidget::fontsUpdatedSlot);
}

BacktraceWidget::~BacktraceWidget() {}

void BacktraceWidget::updateContents()
{
    if (!refreshDeferrer->attemptRefresh(nullptr) || Core()->isDebugTaskInProgress()) {
        return;
    }

    setBacktraceGrid();
}

void BacktraceWidget::setBacktraceGrid()
{
    int i = 0;
    for (CutterJson backtraceItem : Core()->getBacktrace()) {
        QString progCounter = RzAddressString(backtraceItem["pc"].toRVA());
        QString stackPointer = RzAddressString(backtraceItem["sp"].toRVA());
        st64 frameSize = backtraceItem["frame_size"].toSt64();
        QString funcName = backtraceItem["fname"].toString();
        QString desc = backtraceItem["desc"].toString();

        QStandardItem *rowPC = new QStandardItem(progCounter);
        QStandardItem *rowSP = new QStandardItem(stackPointer);
        QStandardItem *rowFrameSize = new QStandardItem(QString::number(frameSize));
        QStandardItem *rowFuncName = new QStandardItem(funcName);
        QStandardItem *rowDesc = new QStandardItem(desc);

        modelBacktrace->setItem(i, 0, rowFuncName);
        modelBacktrace->setItem(i, 1, rowSP);
        modelBacktrace->setItem(i, 2, rowPC);
        modelBacktrace->setItem(i, 3, rowDesc);
        modelBacktrace->setItem(i, 4, rowFrameSize);
        i++;
    }

    // Remove irrelevant old rows
    if (modelBacktrace->rowCount() > i) {
        modelBacktrace->removeRows(i, modelBacktrace->rowCount() - i);
    }

    viewBacktrace->setModel(modelBacktrace);
    viewBacktrace->resizeColumnsToContents();
}

void BacktraceWidget::fontsUpdatedSlot()
{
    viewBacktrace->setFont(Config()->getFont());
}
