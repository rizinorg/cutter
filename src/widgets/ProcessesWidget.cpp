#include <QShortcut>
#include "ProcessesWidget.h"
#include "ui_ProcessesWidget.h"
#include "common/JsonModel.h"
#include "QuickFilterView.h"
#include <rz_debug.h>

#include "core/MainWindow.h"

#define DEBUGGED_PID (-1)

enum ColumnIndex {
    COLUMN_PID = 0,
    COLUMN_UID,
    COLUMN_STATUS,
    COLUMN_PATH,
};

ProcessesWidget::ProcessesWidget(MainWindow *main)
    : CutterDockWidget(main), ui(new Ui::ProcessesWidget)
{
    ui->setupUi(this);

    // Setup processes model
    modelProcesses = new QStandardItemModel(1, 4, this);
    modelProcesses->setHorizontalHeaderItem(COLUMN_PID, new QStandardItem(tr("PID")));
    modelProcesses->setHorizontalHeaderItem(COLUMN_UID, new QStandardItem(tr("UID")));
    modelProcesses->setHorizontalHeaderItem(COLUMN_STATUS, new QStandardItem(tr("Status")));
    modelProcesses->setHorizontalHeaderItem(COLUMN_PATH, new QStandardItem(tr("Path")));
    ui->viewProcesses->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->viewProcesses->verticalHeader()->setVisible(false);
    ui->viewProcesses->setFont(Config()->getFont());

    modelFilter = new ProcessesFilterModel(this);
    modelFilter->setSourceModel(modelProcesses);
    ui->viewProcesses->setModel(modelFilter);

    // CTRL+F switches to the filter view and opens it in case it's hidden
    QShortcut *searchShortcut = new QShortcut(QKeySequence::Find, this);
    connect(searchShortcut, &QShortcut::activated, ui->quickFilterView,
            &QuickFilterView::showFilter);
    searchShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    // ESC switches back to the processes table and clears the buffer
    QShortcut *clearShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(clearShortcut, &QShortcut::activated, this, [this]() {
        ui->quickFilterView->clearFilter();
        ui->viewProcesses->setFocus();
    });
    clearShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    refreshDeferrer = createRefreshDeferrer([this]() { updateContents(); });

    connect(ui->quickFilterView, &QuickFilterView::filterTextChanged, modelFilter,
            &ProcessesFilterModel::setFilterWildcard);
    connect(Core(), &CutterCore::refreshAll, this, &ProcessesWidget::updateContents);
    connect(Core(), &CutterCore::registersChanged, this, &ProcessesWidget::updateContents);
    connect(Core(), &CutterCore::debugTaskStateChanged, this, &ProcessesWidget::updateContents);
    // Seek doesn't necessarily change when switching processes
    connect(Core(), &CutterCore::switchedProcess, this, &ProcessesWidget::updateContents);
    connect(Config(), &Configuration::fontsUpdated, this, &ProcessesWidget::fontsUpdatedSlot);
    connect(ui->viewProcesses, &QTableView::activated, this, &ProcessesWidget::onActivated);
}

ProcessesWidget::~ProcessesWidget() {}

void ProcessesWidget::updateContents()
{
    if (!refreshDeferrer->attemptRefresh(nullptr)) {
        return;
    }

    if (!Core()->currentlyDebugging) {
        // Remove rows from the previous debugging session
        modelProcesses->removeRows(0, modelProcesses->rowCount());
        return;
    }

    if (Core()->isDebugTaskInProgress()) {
        ui->viewProcesses->setDisabled(true);
    } else {
        setProcessesGrid();
        ui->viewProcesses->setDisabled(false);
    }
}

QString ProcessesWidget::translateStatus(QString status)
{
    switch (status.toStdString().c_str()[0]) {
    case RZ_DBG_PROC_STOP:
        return "Stopped";
    case RZ_DBG_PROC_RUN:
        return "Running";
    case RZ_DBG_PROC_SLEEP:
        return "Sleeping";
    case RZ_DBG_PROC_ZOMBIE:
        return "Zombie";
    case RZ_DBG_PROC_DEAD:
        return "Dead";
    case RZ_DBG_PROC_RAISED:
        return "Raised event";
    default:
        return "Unknown status";
    }
}

void ProcessesWidget::setProcessesGrid()
{
    QJsonArray processesValues = Core()->getChildProcesses(DEBUGGED_PID).array();
    int i = 0;
    QFont font;

    for (const QJsonValue &value : processesValues) {
        QJsonObject processesItem = value.toObject();
        int pid = processesItem["pid"].toVariant().toInt();
        int uid = processesItem["uid"].toVariant().toInt();
        QString status = translateStatus(processesItem["status"].toString());
        QString path = processesItem["path"].toString();
        bool current = processesItem["current"].toBool();

        // Use bold font to highlight active thread
        font.setBold(current);

        QStandardItem *rowPid = new QStandardItem(QString::number(pid));
        QStandardItem *rowUid = new QStandardItem(QString::number(uid));
        QStandardItem *rowStatus = new QStandardItem(status);
        QStandardItem *rowPath = new QStandardItem(path);

        rowPid->setFont(font);
        rowUid->setFont(font);
        rowStatus->setFont(font);
        rowPath->setFont(font);

        modelProcesses->setItem(i, COLUMN_PID, rowPid);
        modelProcesses->setItem(i, COLUMN_UID, rowUid);
        modelProcesses->setItem(i, COLUMN_STATUS, rowStatus);
        modelProcesses->setItem(i, COLUMN_PATH, rowPath);
        i++;
    }

    // Remove irrelevant old rows
    if (modelProcesses->rowCount() > i) {
        modelProcesses->removeRows(i, modelProcesses->rowCount() - i);
    }

    modelFilter->setSourceModel(modelProcesses);
    ui->viewProcesses->resizeColumnsToContents();
    ;
}

void ProcessesWidget::fontsUpdatedSlot()
{
    ui->viewProcesses->setFont(Config()->getFont());
}

void ProcessesWidget::onActivated(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    int pid = modelFilter->data(index.sibling(index.row(), COLUMN_PID)).toInt();

    // Verify that the selected pid is still in the processes list since dp= will
    // attach to any given id. If it isn't found simply update the UI.
    QJsonArray processesValues = Core()->getChildProcesses(DEBUGGED_PID).array();
    for (QJsonValue value : processesValues) {
        QString status = value.toObject()["status"].toString();
        if (pid == value.toObject()["pid"].toInt()) {
            if (QString(QChar(RZ_DBG_PROC_ZOMBIE)) == status
                || QString(QChar(RZ_DBG_PROC_DEAD)) == status) {
                QMessageBox msgBox;
                msgBox.setText(tr("Unable to switch to the requested process."));
                msgBox.exec();
            } else {
                Core()->setCurrentDebugProcess(pid);
            }
            break;
        }
    }

    updateContents();
}

ProcessesFilterModel::ProcessesFilterModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

bool ProcessesFilterModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    // All columns are checked for a match
    for (int i = COLUMN_PID; i <= COLUMN_PATH; ++i) {
        QModelIndex index = sourceModel()->index(row, i, parent);
        if (qhelpers::filterStringContains(sourceModel()->data(index).toString(), this)) {
            return true;
        }
    }

    return false;
}
