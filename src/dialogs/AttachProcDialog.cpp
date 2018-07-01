#include "MainWindow.h"
#include "Cutter.h"
#include "AttachProcDialog.h"
#include "ui_AttachProcDialog.h"

#include "utils/Helpers.h"

ProcessModel::ProcessModel(QList<ProcessDescription> *processes, QObject *parent)
    : QAbstractListModel(parent),
      processes(processes)
{
}

int ProcessModel::rowCount(const QModelIndex &) const
{
    return processes->count();
}

int ProcessModel::columnCount(const QModelIndex &) const
{
    return ProcessModel::ColumnCount;
}

QVariant ProcessModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= processes->count())
        return QVariant();

    const ProcessDescription &proc = processes->at(index.row());

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

void ProcessModel::beginReloadProcess()
{
    beginResetModel();
}

void ProcessModel::endReloadProcess()
{
    endResetModel();
}

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
    ProcessDescription leftProc = left.data(ProcessModel::ProcDescriptionRole).value<ProcessDescription>();
    ProcessDescription rightProc = right.data(ProcessModel::ProcDescriptionRole).value<ProcessDescription>();

    switch (left.column()) {
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

AttachProcDialog::AttachProcDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AttachProcDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    processes = Core()->getAllProcesses();
    processModel = new ProcessModel(&processes, this);
    processProxyModel = new ProcessProxyModel(processModel, this);
    ui->procTreeView->setModel(processProxyModel);
    ui->procTreeView->sortByColumn(ProcessModel::PidColumn, Qt::AscendingOrder);
    connect(ui->filterLineEdit, SIGNAL(textChanged(const QString &)), processProxyModel,
            SLOT(setFilterWildcard(const QString &)));
    qhelpers::setVerticalScrollMode(ui->procTreeView);

    // focus on filter line
    ui->filterLineEdit->setFocus();
    // Event filter for capturing Ctrl/Cmd+Return
    ui->filterLineEdit->installEventFilter(this);
}

AttachProcDialog::~AttachProcDialog() {}

void AttachProcDialog::on_buttonBox_accepted()
{
}

void AttachProcDialog::on_buttonBox_rejected()
{
    close();
}

int AttachProcDialog::getPID()
{
    ProcessDescription proc = ui->procTreeView->selectionModel()->currentIndex().data(
                    ProcessModel::ProcDescriptionRole).value<ProcessDescription>();
    return proc.pid;
}

bool AttachProcDialog::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);
    if (event -> type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast <QKeyEvent *> (event);

        // Confirm comment by pressing Ctrl/Cmd+Return
        if ((keyEvent -> modifiers() & Qt::ControlModifier) &&
                ((keyEvent -> key() == Qt::Key_Enter) || (keyEvent -> key() == Qt::Key_Return))) {
            this->accept();
            return true;
        }
    }

    return false;
}

void AttachProcDialog::on_procTreeView_doubleClicked(const QModelIndex &index)
{
    ProcessDescription proc = index.data(ProcessModel::ProcDescriptionRole).value<ProcessDescription>();
    accept();
}