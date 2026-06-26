#include "VTablesWidget.h"

#include "common/Helpers.h"
#include "core/MainWindow.h"
#include "shortcuts/ShortcutManager.h"
#include "ui_VTablesWidget.h"

#include <QModelIndex>
#include <QShortcut>

VTableModel::VTableModel(QObject *parent) : QAbstractItemModel(parent) {}

QModelIndex VTableModel::index(int row, int column, const QModelIndex &parent) const
{
    return createIndex(row, column, (quintptr)parent.isValid() ? parent.row() : -1);
}

QModelIndex VTableModel::parent(const QModelIndex &index) const
{
    return index.isValid() && index.internalId() != (quintptr)-1
            ? this->index(index.internalId(), index.column())
            : QModelIndex();
}

int VTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid()
            ? (parent.parent().isValid() ? 0 : vtables.at(parent.row()).methods.count())
            : vtables.count();
}

int VTableModel::columnCount(const QModelIndex &) const
{
    return Columns::COUNT;
}

QVariant VTableModel::data(const QModelIndex &index, int role) const
{
    const QModelIndex parent = index.parent();
    if (parent.isValid()) {
        const BinClassMethodDescription &res = vtables.at(parent.row()).methods.at(index.row());
        switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
            case NAME:
                return res.name.isEmpty() ? tr("No Name found") : res.name;
            case ADDRESS:
                return rzAddressString(res.addr);
            }
            break;
        case vTableDescriptionRole:
            return QVariant::fromValue(res);
        default:
            break;
        }
    } else {
        switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
            case NAME:
                return tr("VTable") + " " + QString::number(index.row() + 1);
            case ADDRESS:
                return rzAddressString(vtables.at(index.row()).addr);
            }
            break;
        case vTableDescriptionRole: {
            const VTableDescription &res = vtables.at(index.row());
            return QVariant::fromValue(res);
        }
        default:
            break;
        }
    }
    return QVariant();
}

QVariant VTableModel::headerData(int section, Qt::Orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case NAME:
            return tr("Name");
        case ADDRESS:
            return tr("Address");
        default:
            break;
        }
        break;
    default:
        break;
    }
    return QVariant();
}

VTableSortFilterProxyModel::VTableSortFilterProxyModel(VTableModel *model, QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(model);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
    setFilterKeyColumn(VTableModel::NAME);
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    setRecursiveFilteringEnabled(true);
#endif
}

bool VTableSortFilterProxyModel::filterAcceptsRow(int source_row,
                                                  const QModelIndex &source_parent) const
{
    if (QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent)) {
        return true;
    }
    if (source_parent.isValid()) {
        return QSortFilterProxyModel::filterAcceptsRow(source_parent.row(), QModelIndex());
    }
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
    else {
        QAbstractItemModel *const model = sourceModel();
        const QModelIndex source = model->index(source_row, 0, QModelIndex());
        const int rows = model->rowCount(source);
        for (int i = 0; i < rows; ++i) {
            if (QSortFilterProxyModel::filterAcceptsRow(i, source)) {
                return true;
            }
        }
    }
#endif
    return false;
}

VTablesWidget::VTablesWidget(MainWindow *main)
    : CutterDockWidget(main),
      ui(new Ui::VTablesWidget),
      model(new VTableModel(this)),
      proxy(new VTableSortFilterProxyModel(model, this)),
      refreshDeferrer(createRefreshDeferrer([this]() { refreshVTables(); }))
{
    ui->setupUi(this);

    ui->vTableTreeView->setModel(proxy);
    ui->vTableTreeView->sortByColumn(VTableModel::ADDRESS, Qt::AscendingOrder);

    // Esc to clear the filter entry
    QShortcut *clearShortcut = Shortcuts()->makeQShortcut("General.clearFilter", this);
    connect(clearShortcut, &QShortcut::activated, ui->quickFilterView,
            &QuickFilterView::clearFilter);
    clearShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    // Ctrl-F to show/hide the filter entry
    QShortcut *searchShortcut = Shortcuts()->makeQShortcut("General.showFilter", this);
    connect(searchShortcut, &QShortcut::activated, ui->quickFilterView,
            &QuickFilterView::showFilter);
    searchShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    connect(ui->quickFilterView, &QuickFilterView::filterTextChanged, proxy,
            &QSortFilterProxyModel::setFilterWildcard);
    connect(ui->quickFilterView, &QuickFilterView::filterClosed, ui->vTableTreeView,
            [this]() { ui->vTableTreeView->setFocus(); });

    connect(ui->quickFilterView, &QuickFilterView::filterTextChanged, this,
            [this] { ui->quickFilterView->setItemCount(proxy->rowCount()); });

    connect(Core(), &CutterCore::codeRebased, this, &VTablesWidget::refreshVTables);
    connect(Core(), &CutterCore::refreshAll, this, &VTablesWidget::refreshVTables);

    connect(ui->vTableTreeView, &QTreeView::doubleClicked, this,
            &VTablesWidget::onVTableTreeViewDoubleClicked);
}

VTablesWidget::~VTablesWidget() {}

void VTablesWidget::refreshVTables()
{
    if (!refreshDeferrer->attemptRefresh(nullptr)) {
        return;
    }

    model->beginResetModel();
    model->vtables = Core()->getAllVTables();
    model->endResetModel();

    qhelpers::adjustColumns(ui->vTableTreeView, 3, 0);

    ui->vTableTreeView->setColumnWidth(0, 200);

    // set the initial item count
    ui->quickFilterView->setItemCount(proxy->rowCount());
}

void VTablesWidget::onVTableTreeViewDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    const QModelIndex parent = index.parent();
    if (parent.isValid()) {
        Core()->seekAndShow(index.data(VTableModel::vTableDescriptionRole)
                                    .value<BinClassMethodDescription>()
                                    .addr);
    } else {
        Core()->seekAndShow(
                index.data(VTableModel::vTableDescriptionRole).value<VTableDescription>().addr);
    }
}
