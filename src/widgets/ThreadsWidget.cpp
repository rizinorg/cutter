#include "ThreadsWidget.h"
#include "ui_ThreadsWidget.h"
#include "common/JsonModel.h"
#include <r_debug.h>

#include "core/MainWindow.h"

#define DEBUGGED_PID (-1)

enum ColumnIndex {
    COLUMN_CURRENT = 0,
    COLUMN_PID,
    COLUMN_STATUS,
    COLUMN_PATH,
};

ThreadsWidget::ThreadsWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::ThreadsWidget)
{
    ui->setupUi(this);

    // Setup threads model
    modelThreads->setHorizontalHeaderItem(COLUMN_CURRENT, new QStandardItem(tr("Current")));
    modelThreads->setHorizontalHeaderItem(COLUMN_PID, new QStandardItem(tr("PID")));
    modelThreads->setHorizontalHeaderItem(COLUMN_STATUS, new QStandardItem(tr("Status")));
    modelThreads->setHorizontalHeaderItem(COLUMN_PATH, new QStandardItem(tr("Path")));
    viewThreads->setFont(Config()->getFont());
    viewThreads->setModel(modelThreads);
    viewThreads->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->verticalLayout->addWidget(viewThreads);

    refreshDeferrer = createRefreshDeferrer([this]() {
        updateContents();
    });

    viewThreads->setEditTriggers(QAbstractItemView::NoEditTriggers);
    viewThreads->setSortingEnabled(false);

    connect(Core(), &CutterCore::refreshAll, this, &ThreadsWidget::updateContents);
    connect(Core(), &CutterCore::seekChanged, this, &ThreadsWidget::updateContents);
    connect(Config(), &Configuration::fontsUpdated, this, &ThreadsWidget::fontsUpdatedSlot);
    connect(viewThreads, SIGNAL(doubleClicked(const QModelIndex &)), this,
            SLOT(onDoubleClicked(const QModelIndex &)));
}

ThreadsWidget::~ThreadsWidget() {}

void ThreadsWidget::updateContents()
{
    if (!refreshDeferrer->attemptRefresh(nullptr)) {
        return;
    }
    setThreadsGrid();
}

QString ThreadsWidget::translateStatus(QString status)
{
    switch (status.toStdString().c_str()[0]) {
    case R_DBG_PROC_STOP:
        return "Stopped";
    case R_DBG_PROC_RUN:
        return "Running";
    case R_DBG_PROC_SLEEP:
        return "Sleeping";
    case R_DBG_PROC_ZOMBIE:
        return "Zombie";
    case R_DBG_PROC_DEAD:
        return "Dead";
    case R_DBG_PROC_RAISED:
        return "Raised event";
    }
}

void ThreadsWidget::setThreadsGrid()
{
    QJsonArray threadsValues = Core()->getProcessThreads(DEBUGGED_PID).array();
    int i = 0;
    for (const QJsonValue &value : threadsValues) {
        QJsonObject threadsItem = value.toObject();
        int pid = threadsItem["pid"].toVariant().toInt();
        QString status = translateStatus(threadsItem["status"].toString());
        QString path = threadsItem["path"].toString();
        bool current = threadsItem["current"].toBool();

        QStandardItem *rowCurrent = new QStandardItem(QString(current ? "True" : "False"));
        QStandardItem *rowPid = new QStandardItem(QString::number(pid));
        QStandardItem *rowStatus = new QStandardItem(status);
        QStandardItem *rowPath = new QStandardItem(path);

        modelThreads->setItem(i, COLUMN_CURRENT, rowCurrent);
        modelThreads->setItem(i, COLUMN_PID, rowPid);
        modelThreads->setItem(i, COLUMN_STATUS, rowStatus);
        modelThreads->setItem(i, COLUMN_PATH, rowPath);
        i++;
    }

    viewThreads->setModel(modelThreads);
    viewThreads->resizeColumnsToContents();;
}

void ThreadsWidget::fontsUpdatedSlot()
{
    viewThreads->setFont(Config()->getFont());
}

void ThreadsWidget::onDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    int row = index.row();

    QJsonArray threadsValues = Core()->getProcessThreads(DEBUGGED_PID).array();
    int tid = threadsValues[row].toObject()["pid"].toInt();

    Core()->setCurrentDebugThread(tid);

    updateContents();
}
