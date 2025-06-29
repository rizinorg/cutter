#include <QShortcut>
#include "ThreadsWidget.h"
#include "CutterCommon.h"
#include "Helpers.h"
#include "ui_ThreadsWidget.h"
#include "QuickFilterView.h"
#include <rz_debug.h>

#include "core/MainWindow.h"
#include "shortcuts/ShortcutManager.h"

ThreadModel::ThreadModel(QObject *parent) : QAbstractListModel(parent) {}

void ThreadModel::setList(QList<ThreadDescription> data)
{
    beginResetModel();
    this->threads = data;
    endResetModel();
}

int ThreadModel::rowCount(const QModelIndex &) const
{
    return threads.count();
}

int ThreadModel::columnCount(const QModelIndex &) const
{
    return ThreadModel::ColumnIndex::COLUMN_COUNT;
}

QVariant ThreadModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= threads.count())
        return QVariant();

    const ThreadDescription &thread = threads.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case ColumnIndex::COLUMN_PID:
            return thread.current ? QString("*%0").arg(thread.pid) : QString("%0").arg(thread.pid);
        case ColumnIndex::COLUMN_STATUS:
            return translateStatus(thread.status);
        case ColumnIndex::COLUMN_PATH:
            return thread.path;
        case ColumnIndex::COLUMN_PC:
            return RzAddressString(thread.pc);
        case ColumnIndex::COLUMN_TLS:
            return RzAddressString(thread.tls);
        default:
            return QVariant();
        }
    case Qt::FontRole: {
        if (thread.current) {
            QFont font;
            font.setBold(true);
            return font;
        }
        return QVariant();
    }
    case Qt::EditRole:
        switch (index.column()) {
        case ColumnIndex::COLUMN_PID:
            return thread.pid;
        case ColumnIndex::COLUMN_PC:
            return thread.pc;
        case ColumnIndex::COLUMN_TLS:
            return thread.tls;
        default:
            return data(index, Qt::DisplayRole);
        }
        break;
    default:
        return QVariant();
    }
}

QVariant ThreadModel::headerData(int section, Qt::Orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case ColumnIndex::COLUMN_PID:
            return tr("TID");
        case ColumnIndex::COLUMN_STATUS:
            return tr("Status");
        case ColumnIndex::COLUMN_PATH:
            return tr("Path");
        case ColumnIndex::COLUMN_PC:
            return tr("PC");
        case ColumnIndex::COLUMN_TLS:
            return tr("TLS");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

ThreadsWidget::ThreadsWidget(MainWindow *main)
    : CutterDockWidget(main),
      ui(new Ui::ThreadsWidget),
      menuText(this),
      addressableItemContextMenu(this, main)
{
    ui->setupUi(this);

    // Setup threads model
    modelThreads = new ThreadModel(this);

    ui->viewThreads->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->viewThreads->verticalHeader()->setVisible(false);
    fontsUpdatedSlot();

    modelFilter = new QSortFilterProxyModel(this);
    modelFilter->setSourceModel(modelThreads);

    modelFilter->setFilterCaseSensitivity(Qt::CaseInsensitive);
    modelFilter->setSortCaseSensitivity(Qt::CaseInsensitive);
    modelFilter->setFilterKeyColumn(-1);
    modelFilter->setFilterRole(Qt::DisplayRole);
    modelFilter->setSortRole(Qt::EditRole);

    ui->viewThreads->setModel(modelFilter);

    // CTRL+F switches to the filter view and opens it in case it's hidden
    QShortcut *searchShortcut = Shortcuts()->makeQShortcut("General.showFilter", this);
    connect(searchShortcut, &QShortcut::activated, ui->quickFilterView,
            &QuickFilterView::showFilter);
    searchShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    // ESC switches back to the thread table and clears the buffer
    QShortcut *clearShortcut = Shortcuts()->makeQShortcut("General.clearFilter", this);
    connect(clearShortcut, &QShortcut::activated, this, [this]() {
        ui->quickFilterView->clearFilter();
        ui->viewThreads->setFocus();
    });
    clearShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    refreshDeferrer = createRefreshDeferrer([this]() { updateContents(); });

    menuText.setSeparator(true);
    qhelpers::prependQAction(&menuText, &addressableItemContextMenu);

    connect(ui->quickFilterView, &QuickFilterView::filterTextChanged, modelFilter,
            &QSortFilterProxyModel::setFilterWildcard);
    connect(Core(), &CutterCore::refreshAll, this, &ThreadsWidget::updateContents);
    connect(Core(), &CutterCore::registersChanged, this, &ThreadsWidget::updateContents);
    connect(Core(), &CutterCore::debugTaskStateChanged, this, &ThreadsWidget::updateContents);
    // Seek doesn't necessarily change when switching threads/processes
    connect(Core(), &CutterCore::switchedThread, this, &ThreadsWidget::updateContents);
    connect(Core(), &CutterCore::switchedProcess, this, &ThreadsWidget::updateContents);
    connect(Config(), &Configuration::fontsUpdated, this, &ThreadsWidget::fontsUpdatedSlot);
    connect(ui->viewThreads, &QTableView::activated, this, &ThreadsWidget::onActivated);
    connect(ui->viewThreads, &QWidget::customContextMenuRequested, this,
            &ThreadsWidget::tableCustomContextMenu);
    connect(ui->viewThreads->selectionModel(), &QItemSelectionModel::currentChanged, this,
            &ThreadsWidget::onCurrentChanged);
    ui->viewThreads->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
}

ThreadsWidget::~ThreadsWidget() {}

void ThreadsWidget::updateContents()
{
    if (!refreshDeferrer->attemptRefresh(nullptr)) {
        return;
    }

    if (!Core()->currentlyDebugging) {
        // Remove rows from the previous debugging session
        modelThreads->setList(QList<ThreadDescription>());
        return;
    }

    if (Core()->isDebugTaskInProgress()) {
        ui->viewThreads->setDisabled(true);
    } else {
        setThreadsGrid();
        ui->viewThreads->setDisabled(false);
    }
}

QString ThreadModel::translateStatus(const char status) const
{
    switch (status) {
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

void ThreadsWidget::setThreadsGrid()
{
    modelThreads->setList(Core()->getProcessThreads());
    ui->viewThreads->resizeColumnsToContents();
}

void ThreadsWidget::fontsUpdatedSlot()
{
    ui->viewThreads->setFont(Config()->getFont());
}

void ThreadsWidget::onActivated(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    int tid = modelFilter->data(index.sibling(index.row(), ThreadModel::COLUMN_PID), Qt::EditRole)
                      .toInt();

    // Verify that the selected tid is still in the threads list since dpt= will
    // attach to any given id. If it isn't found simply update the UI.
    for (const auto &value : Core()->getProcessThreads()) {
        if (tid == value.pid) {
            Core()->setCurrentDebugThread(tid);
            break;
        }
    }

    updateContents();
}

void ThreadsWidget::onCurrentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous)

    RVA offset = 0;
    if (current.column() == ThreadModel::ColumnIndex::COLUMN_TLS) {
        offset = current.data(Qt::EditRole).toULongLong();
        menuText.setText(tr("TLS (%0)").arg(RzAddressString(offset)));
    } else {
        offset = current.sibling(current.row(), ThreadModel::ColumnIndex::COLUMN_PC)
                         .data(Qt::EditRole)
                         .toULongLong();
        menuText.setText(tr("PC (%0)").arg(RzAddressString(offset)));
    }

    addressableItemContextMenu.setTarget(offset);
}

void ThreadsWidget::tableCustomContextMenu(const QPoint &pos)
{
    addressableItemContextMenu.exec(this->ui->viewThreads->viewport()->mapToGlobal(pos));
}
