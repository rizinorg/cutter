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
    RzList *list = rz_core_debug_backtraces(Core()->core());
    int i = 0;
    RzListIter *iter;
    RzBacktrace *bt;
    CutterRzListForeach (list, iter, RzBacktrace, bt) {
        QString funcName = bt->fcn ? bt->fcn->name : "";
        QString pc = RzAddressString(bt->frame ? bt->frame->addr : 0);
        QString sp = RzAddressString(bt->frame ? bt->frame->sp : 0);
        QString frameSize = QString::number(bt->frame ? bt->frame->size : 0);
        QString desc = bt->desc;

        modelBacktrace->setItem(i, 0, new QStandardItem(funcName));
        modelBacktrace->setItem(i, 1, new QStandardItem(sp));
        modelBacktrace->setItem(i, 2, new QStandardItem(pc));
        modelBacktrace->setItem(i, 3, new QStandardItem(desc));
        modelBacktrace->setItem(i, 4, new QStandardItem(frameSize));
        ++i;
    }
    rz_list_free(list);

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
