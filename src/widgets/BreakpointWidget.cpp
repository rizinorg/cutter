#include "BreakpointWidget.h"
#include "ui_BreakpointWidget.h"
#include "dialogs/BreakpointsDialog.h"
#include "core/MainWindow.h"
#include "common/Helpers.h"
#include <QMenu>

BreakpointModel::BreakpointModel(QList<BreakpointDescription> *breakpoints, QObject *parent)
    : QAbstractListModel(parent),
      breakpoints(breakpoints)
{
}

int BreakpointModel::rowCount(const QModelIndex &) const
{
    return breakpoints->count();
}

int BreakpointModel::columnCount(const QModelIndex &) const
{
    return BreakpointModel::ColumnCount;
}

QVariant BreakpointModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= breakpoints->count())
        return QVariant();

    const BreakpointDescription &breakpoint = breakpoints->at(index.row());

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

BreakpointProxyModel::BreakpointProxyModel(BreakpointModel *sourceModel, QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(sourceModel);
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

    breakpointModel = new BreakpointModel(&breakpoints, this);
    breakpointProxyModel = new BreakpointProxyModel(breakpointModel, this);
    ui->breakpointTreeView->setModel(breakpointProxyModel);
    ui->breakpointTreeView->sortByColumn(BreakpointModel::AddrColumn, Qt::AscendingOrder);

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

    connect(Core(), &CutterCore::refreshAll, this, &BreakpointWidget::refreshBreakpoint);
    connect(Core(), &CutterCore::breakpointsChanged, this, &BreakpointWidget::refreshBreakpoint);
    connect(Core(), &CutterCore::codeRebased, this, &BreakpointWidget::refreshBreakpoint);
    connect(Core(), &CutterCore::refreshCodeViews, this, &BreakpointWidget::refreshBreakpoint);
    connect(ui->addBreakpoint, &QAbstractButton::clicked, this, &BreakpointWidget::addBreakpointDialog);
    connect(ui->delBreakpoint, &QAbstractButton::clicked, this, &BreakpointWidget::delBreakpoint);
    connect(ui->delAllBreakpoints, &QAbstractButton::clicked, Core(), &CutterCore::delAllBreakpoints);
    ui->breakpointTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->breakpointTreeView, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showBreakpointContextMenu(const QPoint &)));
}

BreakpointWidget::~BreakpointWidget() = default;

void BreakpointWidget::refreshBreakpoint()
{
    if (!refreshDeferrer->attemptRefresh(nullptr)) {
        return;
    }

    breakpointModel->beginResetModel();
    breakpoints = Core()->getBreakpoints();
    breakpointModel->endResetModel();

    ui->breakpointTreeView->resizeColumnToContents(0);
    ui->breakpointTreeView->resizeColumnToContents(1);
    ui->breakpointTreeView->resizeColumnToContents(2);
}

void BreakpointWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->breakpointTreeView);
}

void BreakpointWidget::on_breakpointTreeView_doubleClicked(const QModelIndex &index)
{
    BreakpointDescription item = index.data(
                                     BreakpointModel::BreakpointDescriptionRole).value<BreakpointDescription>();
    Core()->seekAndShow(item.addr);
}

void BreakpointWidget::showBreakpointContextMenu(const QPoint &pt)
{
    QMenu *menu = new QMenu(ui->breakpointTreeView);
    menu->clear();
    menu->addAction(actionDelBreakpoint);
    menu->addAction(actionToggleBreakpoint);

    menu->exec(ui->breakpointTreeView->viewport()->mapToGlobal(pt));
    this->setContextMenuPolicy(Qt::CustomContextMenu);

    delete menu;
}

void BreakpointWidget::addBreakpointDialog()
{
    BreakpointsDialog dialog(this);

    if (dialog.exec()) {
        QString bps = dialog.getBreakpoints();
        if (!bps.isEmpty()) {
            QStringList bpList = bps.split(QLatin1Char(' '), QString::SkipEmptyParts);
            for (const QString &bp : bpList) {
                Core()->toggleBreakpoint(bp);
            }
        }
    }
}

void BreakpointWidget::delBreakpoint()
{
    BreakpointDescription bp = ui->breakpointTreeView->selectionModel()->currentIndex().data(
                                   BreakpointModel::BreakpointDescriptionRole).value<BreakpointDescription>();
    Core()->delBreakpoint(bp.addr);
}

void BreakpointWidget::toggleBreakpoint()
{
    BreakpointDescription bp = ui->breakpointTreeView->selectionModel()->currentIndex().data(
                                   BreakpointModel::BreakpointDescriptionRole).value<BreakpointDescription>();
    if (bp.enabled) {
        Core()->disableBreakpoint(bp.addr);
    } else {
        Core()->enableBreakpoint(bp.addr);
    }
}
