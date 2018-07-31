#include "MainWindow.h"
#include "Cutter.h"
#include "AttachProcDialog.h"
#include "ui_AttachProcDialog.h"

#include "utils/Helpers.h"

#include <QScrollBar>

// ------------
// ProcessModel
// ------------
ProcessModel::ProcessModel(QObject *parent)
    : QAbstractListModel(parent)
{
    updateData();
}

void ProcessModel::updateData()
{
    beginResetModel();

    processes = Core()->getAllProcesses();

    endResetModel();
}

int ProcessModel::rowCount(const QModelIndex &) const
{
    return processes.count();
}

int ProcessModel::columnCount(const QModelIndex &) const
{
    return ProcessModel::ColumnCount;
}

QVariant ProcessModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= processes.count())
        return QVariant();

    const ProcessDescription &proc = processes.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case PidColumn:
            return proc.pid;
        case UidColumn:
            return proc.uid;
        case StatusColumn:
            return proc.status;
        case PathColumn:
            return proc.path;
        default:
            return QVariant();
        }
    case ProcDescriptionRole:
        return QVariant::fromValue(proc);
    default:
        return QVariant();
    }
}

QVariant ProcessModel::headerData(int section, Qt::Orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case PidColumn:
            return tr("PID");
        case UidColumn:
            return tr("UID");
        case StatusColumn:
            return tr("Status");
        case PathColumn:
            return tr("Path");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

bool ProcessModel::lessThan(const ProcessDescription &leftProc, const ProcessDescription &rightProc,
                            int column)
{
    switch (column) {
    case ProcessModel::PidColumn:
        return leftProc.pid < rightProc.pid;
    case ProcessModel::UidColumn:
        return leftProc.uid < rightProc.uid;
    case ProcessModel::StatusColumn:
        return leftProc.status < rightProc.status;
    case ProcessModel::PathColumn:
        return leftProc.path < rightProc.path;
    default:
        break;
    }

    return leftProc.pid < rightProc.pid;
}

// ------------------------------
// ProcessBeingAnalysedProxyModel
// ------------------------------
ProcessBeingAnalysedProxyModel::ProcessBeingAnalysedProxyModel(ProcessModel *sourceModel,
                                                               QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(sourceModel);

    // @SEE: Should there be a getFilename() in Core()? Not the first time I use this
    processBeingAnalysedFilename = processPathToFilename(Core()->getConfig("file.path"));
}

QString ProcessBeingAnalysedProxyModel::processPathToFilename(const QString &path) const
{
    // removes the arguments and gets filename from the process path
    return path.split(" ").first().split("/").last();
}

bool ProcessBeingAnalysedProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    QModelIndex index = sourceModel()->index(row, 0, parent);
    ProcessDescription item = index.data(ProcessModel::ProcDescriptionRole).value<ProcessDescription>();

    QString procFilename = processPathToFilename(item.path);
    return procFilename == processBeingAnalysedFilename;
}

bool ProcessBeingAnalysedProxyModel::lessThan(const QModelIndex &left,
                                              const QModelIndex &right) const
{
    ProcessDescription leftProc = left.data(
                                      ProcessModel::ProcDescriptionRole).value<ProcessDescription>();
    ProcessDescription rightProc = right.data(
                                       ProcessModel::ProcDescriptionRole).value<ProcessDescription>();

    return ProcessModel::lessThan(leftProc, rightProc, left.column());
}

// -----------------
// ProcessProxyModel
// -----------------
ProcessProxyModel::ProcessProxyModel(ProcessModel *sourceModel, QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(sourceModel);
}

bool ProcessProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    QModelIndex index = sourceModel()->index(row, 0, parent);
    ProcessDescription item = index.data(ProcessModel::ProcDescriptionRole).value<ProcessDescription>();
    return item.path.contains(filterRegExp());
}

bool ProcessProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    ProcessDescription leftProc = left.data(
                                      ProcessModel::ProcDescriptionRole).value<ProcessDescription>();
    ProcessDescription rightProc = right.data(
                                       ProcessModel::ProcDescriptionRole).value<ProcessDescription>();

    return ProcessModel::lessThan(leftProc, rightProc, left.column());
}

// ----------------
// AttachProcDialog
// ----------------
AttachProcDialog::AttachProcDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AttachProcDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    processModel = new ProcessModel(this);
    processProxyModel = new ProcessProxyModel(processModel, this);
    processBeingAnalyzedProxyModel = new ProcessBeingAnalysedProxyModel(processModel, this);

    // View of all processes
    auto allView = ui->allProcView;
    allView->setModel(processProxyModel);
    allView->sortByColumn(ProcessModel::PidColumn, Qt::DescendingOrder);

    // View of the processes with the same name as the one being analyzed
    auto smallView = ui->procBeingAnalyzedView;
    smallView->setModel(processBeingAnalyzedProxyModel);
    smallView->setCurrentIndex(smallView->model()->index(0, 0));

    // To get the 'FocusIn' events
    allView->installEventFilter(this);
    smallView->installEventFilter(this);

    // focus on filter line
    ui->filterLineEdit->setFocus();
    connect(ui->filterLineEdit, SIGNAL(textChanged(const QString &)), processProxyModel,
            SLOT(setFilterWildcard(const QString &)));

    // Update the processes every 'updateIntervalMs' seconds
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateModelData()));
    timer->start(updateIntervalMs);
}

AttachProcDialog::~AttachProcDialog()
{
    timer->stop();
    delete timer;
    delete processBeingAnalyzedProxyModel;
    delete processProxyModel;
    delete processModel;
}

void AttachProcDialog::updateModelData()
{
    auto allView = ui->allProcView;
    auto smallView = ui->procBeingAnalyzedView;

    // Save the old selection and scroll position so that we can update and
    // model and then restore it.
    bool allViewHadSelection = allView->selectionModel()->hasSelection();
    bool smallViewHadSelection = smallView->selectionModel()->hasSelection();
    int allViewPrevScrollPos = 0;
    int smallViewPrevScrollPos = 0;
    int allViewPrevPID = 0;
    int smallViewPrevPID = 0;

    if (allViewHadSelection) {
        allViewPrevScrollPos = allView->verticalScrollBar()->value();
        allViewPrevPID = allView->selectionModel()->currentIndex().data(
                             ProcessModel::ProcDescriptionRole).value<ProcessDescription>().pid;
    }
    if (smallViewHadSelection) {
        smallViewPrevScrollPos = smallView->verticalScrollBar()->value();
        smallViewPrevPID = smallView->selectionModel()->currentIndex().data(
                               ProcessModel::ProcDescriptionRole).value<ProcessDescription>().pid;
    }

    // Let the model update
    processModel->updateData();

    // Restore the selection and scroll position
    if (allViewHadSelection) {
        QModelIndexList idx = allView->model()->match(
                                  allView->model()->index(0, 0), Qt::DisplayRole, QVariant::fromValue(allViewPrevPID));
        if (!idx.isEmpty()) {
            allView->setCurrentIndex(idx.first());
            allView->verticalScrollBar()->setValue(allViewPrevScrollPos);
        }
    }
    if (smallViewHadSelection) {
        QModelIndexList idx = smallView->model()->match(
                                  smallView->model()->index(0, 0), Qt::DisplayRole, QVariant::fromValue(smallViewPrevPID));

        if (!idx.isEmpty()) {
            smallView->setCurrentIndex(idx.first());
            smallView->verticalScrollBar()->setValue(smallViewPrevScrollPos);
        }
    }

    // Init selection if nothing was ever selected yet, and a new process with the same name
    // as the one being analysed was launched.
    if (!allView->selectionModel()->hasSelection() && !smallView->selectionModel()->hasSelection()) {
        smallView->setCurrentIndex(smallView->model()->index(0, 0));
    }
}

void AttachProcDialog::on_buttonBox_accepted()
{
}

void AttachProcDialog::on_buttonBox_rejected()
{
    close();
}

bool AttachProcDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::FocusIn) {
        if (obj == ui->allProcView) {
            ui->procBeingAnalyzedView->selectionModel()->clearSelection();
            wasAllProcViewLastPressed = true;
            return true;
        } else if (obj == ui->procBeingAnalyzedView) {
            ui->allProcView->selectionModel()->clearSelection();
            wasAllProcViewLastPressed = false;
            return true;
        }
    }

    return false;
}

int AttachProcDialog::getPID()
{
    int pid;

    // Here we need to know which table was selected last to get the proper PID
    if (wasAllProcViewLastPressed && ui->allProcView->selectionModel()->hasSelection()) {
        pid = ui->allProcView->selectionModel()->currentIndex().
              data(ProcessModel::ProcDescriptionRole).value<ProcessDescription>().pid;
    } else if (!wasAllProcViewLastPressed
               && ui->procBeingAnalyzedView->selectionModel()->hasSelection()) {
        pid = ui->procBeingAnalyzedView->selectionModel()->currentIndex().
              data(ProcessModel::ProcDescriptionRole).value<ProcessDescription>().pid;
    } else {
        // Error attaching. No process selected! Happens when you press ENTER but
        // there was no process with the same name as the one being analyzed.
        pid = -1;
    }

    return pid;
}

void AttachProcDialog::on_allProcView_doubleClicked(const QModelIndex &index)
{
    Q_UNUSED(index);

    accept();
}

void AttachProcDialog::on_procBeingAnalyzedView_doubleClicked(const QModelIndex &index)
{
    Q_UNUSED(index);

    accept();
}