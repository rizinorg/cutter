#include "BreakpointWidget.h"
#include "ui_BreakpointWidget.h"
#include "dialogs/BreakpointsDialog.h"
#include "core/MainWindow.h"
#include "common/Helpers.h"
#include "widgets/BoolToggleDelegate.h"
#include <QMenu>
#include <QStyledItemDelegate>
#include <QCheckBox>

BreakpointModel::BreakpointModel(QObject *parent)
    : AddressableItemModel<QAbstractListModel>(parent)
{
}

void BreakpointModel::refresh()
{
    beginResetModel();
    breakpoints = Core()->getBreakpoints();
    endResetModel();
}

int BreakpointModel::rowCount(const QModelIndex &) const
{
    return breakpoints.count();
}

int BreakpointModel::columnCount(const QModelIndex &) const
{
    return BreakpointModel::ColumnCount;
}

QVariant BreakpointModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= breakpoints.count())
        return QVariant();

    const BreakpointDescription &breakpoint = breakpoints.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case AddrColumn:
            return RAddressString(breakpoint.addr);
        case PermColumn:
            return breakpoint.permission;
        case HwColumn:
            return breakpoint.hw;
        case TraceColumn:
            return breakpoint.trace;
        case EnabledColumn:
            return breakpoint.enabled;
        default:
            return QVariant();
        }
    case Qt::EditRole:
        switch (index.column()) {
        case TraceColumn:
            return breakpoint.trace;
        case EnabledColumn:
            return breakpoint.enabled;
        default:
            return data(index, Qt::DisplayRole);
        }
    case BreakpointDescriptionRole:
        return QVariant::fromValue(breakpoint);
    default:
        return QVariant();
    }
}

QVariant BreakpointModel::headerData(int section, Qt::Orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case AddrColumn:
            return tr("Offset");
        case PermColumn:
            return tr("Permissions");
        case HwColumn:
            return tr("Hardware bp");
        case TraceColumn:
            return tr("Tracing");
        case EnabledColumn:
            return tr("Active");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

bool BreakpointModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.row() >= breakpoints.count())
        return false;

    BreakpointDescription &breakpoint = breakpoints[index.row()];

    switch (role) {
    case Qt::EditRole:
        switch (index.column()) {
        case TraceColumn:
            breakpoint.trace = value.toBool();
            Core()->setBreakpointTrace(index.row(), breakpoint.trace);
            emit dataChanged(index, index, {role, Qt::DisplayRole});
            return true;
        case EnabledColumn:
            breakpoint.enabled = value.toBool();
            if (breakpoint.enabled) {
                Core()->enableBreakpoint(breakpoint.addr);
            } else {
                Core()->disableBreakpoint(breakpoint.addr);
            }
            emit dataChanged(index, index, {role, Qt::DisplayRole});
            return true;
        default:
            return false;
        }

    default:
        return false;
    }
}

Qt::ItemFlags BreakpointModel::flags(const QModelIndex &index) const
{
    switch (index.column()) {
    case TraceColumn:
        return AddressableItemModel::flags(index) | Qt::ItemFlag::ItemIsEditable;
    case EnabledColumn:
        return AddressableItemModel::flags(index) | Qt::ItemFlag::ItemIsEditable;
    default:
        return AddressableItemModel::flags(index);
    }
}

RVA BreakpointModel::address(const QModelIndex &index) const
{
    if (index.row() < breakpoints.count()) {
        return breakpoints.at(index.row()).addr;
    }
    return RVA_INVALID;
}

BreakpointProxyModel::BreakpointProxyModel(BreakpointModel *sourceModel, QObject *parent)
    : AddressableFilterProxyModel(sourceModel, parent)
{
}

bool BreakpointProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    QModelIndex index = sourceModel()->index(row, 0, parent);
    BreakpointDescription item = index.data(
                                     BreakpointModel::BreakpointDescriptionRole).value<BreakpointDescription>();
    return item.permission.contains(filterRegExp());
}

bool BreakpointProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    BreakpointDescription leftBreakpt = left.data(
                                            BreakpointModel::BreakpointDescriptionRole).value<BreakpointDescription>();
    BreakpointDescription rightBreakpt = right.data(
                                             BreakpointModel::BreakpointDescriptionRole).value<BreakpointDescription>();

    switch (left.column()) {
    case BreakpointModel::AddrColumn:
        return leftBreakpt.addr < rightBreakpt.addr;
    case BreakpointModel::HwColumn:
        return leftBreakpt.hw < rightBreakpt.hw;
    case BreakpointModel::PermColumn:
        return leftBreakpt.permission < rightBreakpt.permission;
    case BreakpointModel::EnabledColumn:
        return leftBreakpt.enabled < rightBreakpt.enabled;
    default:
        break;
    }

    return leftBreakpt.addr < rightBreakpt.addr;
}

BreakpointWidget::BreakpointWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::BreakpointWidget)
{
    ui->setupUi(this);

    ui->breakpointTreeView->setMainWindow(mainWindow);
    breakpointModel = new BreakpointModel(this);
    breakpointProxyModel = new BreakpointProxyModel(breakpointModel, this);
    ui->breakpointTreeView->setModel(breakpointProxyModel);
    ui->breakpointTreeView->sortByColumn(BreakpointModel::AddrColumn, Qt::AscendingOrder);
    ui->breakpointTreeView->setItemDelegate(new BoolTogggleDelegate(this));

    refreshDeferrer = createRefreshDeferrer([this]() {
        refreshBreakpoint();
    });

    setScrollMode();

    actionDelBreakpoint = new QAction(tr("Delete breakpoint"), this);
    actionDelBreakpoint->setShortcut(Qt::Key_Delete);
    actionDelBreakpoint->setShortcutContext(Qt::WidgetShortcut);
    connect(actionDelBreakpoint, &QAction::triggered, this, &BreakpointWidget::delBreakpoint);
    ui->breakpointTreeView->addAction(actionDelBreakpoint);

    actionToggleBreakpoint = new QAction(tr("Toggle breakpoint"), this);
    actionToggleBreakpoint->setShortcut(Qt::Key_Space);
    actionToggleBreakpoint->setShortcutContext(Qt::WidgetShortcut);
    connect(actionToggleBreakpoint, &QAction::triggered, this, &BreakpointWidget::toggleBreakpoint);
    ui->breakpointTreeView->addAction(actionToggleBreakpoint);

    auto contextMenu = ui->breakpointTreeView->getItemContextMenu();
    contextMenu->addAction(actionToggleBreakpoint);
    contextMenu->addAction(actionDelBreakpoint);

    connect(Core(), &CutterCore::refreshAll, this, &BreakpointWidget::refreshBreakpoint);
    connect(Core(), &CutterCore::breakpointsChanged, this, &BreakpointWidget::refreshBreakpoint);
    connect(Core(), &CutterCore::codeRebased, this, &BreakpointWidget::refreshBreakpoint);
    connect(Core(), &CutterCore::refreshCodeViews, this, &BreakpointWidget::refreshBreakpoint);
    connect(ui->addBreakpoint, &QAbstractButton::clicked, this, &BreakpointWidget::addBreakpointDialog);
    connect(ui->delBreakpoint, &QAbstractButton::clicked, this, &BreakpointWidget::delBreakpoint);
    connect(ui->delAllBreakpoints, &QAbstractButton::clicked, Core(), &CutterCore::delAllBreakpoints);
}

BreakpointWidget::~BreakpointWidget() = default;

void BreakpointWidget::refreshBreakpoint()
{
    if (editing || !refreshDeferrer->attemptRefresh(nullptr)) {
        return;
    }

    breakpointModel->refresh();

    ui->breakpointTreeView->resizeColumnToContents(0);
    ui->breakpointTreeView->resizeColumnToContents(1);
    ui->breakpointTreeView->resizeColumnToContents(2);
}

void BreakpointWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->breakpointTreeView);
}

void BreakpointWidget::addBreakpointDialog()
{
    BreakpointsDialog dialog(this);

    if (dialog.exec()) {
        QString bps = dialog.getBreakpoints();
        if (!bps.isEmpty()) {
            QStringList bpList = bps.split(QLatin1Char(' '), QString::SkipEmptyParts);
            for (const QString &bp : bpList) {
                Core()->addBreakpoint(bp);
            }
        }
    }
}

QVector<RVA> BreakpointWidget::getSelectedAddresses() const
{
    auto selection = ui->breakpointTreeView->selectionModel()->selectedRows();
    QVector<RVA> breakpointAddressese(selection.count());
    int index = 0;
    for (auto row : selection) {
        breakpointAddressese[index++] = breakpointProxyModel->address(row);
    }
    return breakpointAddressese;
}

void BreakpointWidget::delBreakpoint()
{
    auto breakpointsToRemove = getSelectedAddresses();
    for (auto address : breakpointsToRemove) {
        Core()->delBreakpoint(address);
    }
}

void BreakpointWidget::toggleBreakpoint()
{
    auto selection = ui->breakpointTreeView->selectionModel()->selectedRows();
    editing = true;
    for (auto row : selection) {
        auto cell = breakpointProxyModel->index(row.row(), BreakpointModel::EnabledColumn);
        breakpointProxyModel->setData(cell, !cell.data(Qt::EditRole).toBool());
    }
    editing = false;
}
