#include "FlirtWidget.h"
#include "ui_FlirtWidget.h"
#include "core/MainWindow.h"
#include "common/Helpers.h"

FlirtModel::FlirtModel(QList<FlirtDescription> *sigdb, QObject *parent)
    : QAbstractListModel(parent), sigdb(sigdb)
{
}

int FlirtModel::rowCount(const QModelIndex &) const
{
    return sigdb->count();
}

int FlirtModel::columnCount(const QModelIndex &) const
{
    return FlirtModel::ColumnCount;
}

QVariant FlirtModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= sigdb->count())
        return QVariant();

    const FlirtDescription &entry = sigdb->at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case BinTypeColumn:
            return entry.bin_name;
        case ArchNameColumn:
            return entry.arch_name;
        case ArchBitsColumn:
            return entry.arch_bits;
        case NumModulesColumn:
            return entry.n_modules;
        case NameColumn:
            return entry.base_name;
        case DetailsColumn:
            return entry.details;
        default:
            return QVariant();
        }

    case FlirtDescriptionRole:
        return QVariant::fromValue(entry);

    case Qt::ToolTipRole: {
        return entry.short_path;
    }

    default:
        return QVariant();
    }
}

QVariant FlirtModel::headerData(int section, Qt::Orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case BinTypeColumn:
            return tr("Bin");
        case ArchNameColumn:
            return tr("Arch");
        case ArchBitsColumn:
            return tr("Bits");
        case NumModulesColumn:
            return tr("# Funcs");
        case NameColumn:
            return tr("Name");
        case DetailsColumn:
            return tr("Details");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

FlirtProxyModel::FlirtProxyModel(FlirtModel *sourceModel, QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(sourceModel);
}

bool FlirtProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    QModelIndex index = sourceModel()->index(row, 0, parent);
    FlirtDescription entry = index.data(FlirtModel::FlirtDescriptionRole).value<FlirtDescription>();
    return qhelpers::filterStringContains(entry.base_name, this);
}

bool FlirtProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    FlirtDescription leftEntry =
            left.data(FlirtModel::FlirtDescriptionRole).value<FlirtDescription>();
    FlirtDescription rightEntry =
            right.data(FlirtModel::FlirtDescriptionRole).value<FlirtDescription>();

    switch (left.column()) {
    case FlirtModel::BinTypeColumn:
        return leftEntry.bin_name < rightEntry.bin_name;
    case FlirtModel::ArchNameColumn:
        return leftEntry.arch_name < rightEntry.arch_name;
    case FlirtModel::ArchBitsColumn:
        return leftEntry.arch_bits.toULongLong() < rightEntry.arch_bits.toULongLong();
    case FlirtModel::NumModulesColumn:
        return leftEntry.n_modules.toULongLong() < rightEntry.n_modules.toULongLong();
    case FlirtModel::NameColumn:
        return leftEntry.base_name < rightEntry.base_name;
    case FlirtModel::DetailsColumn:
        return leftEntry.details < rightEntry.details;
    default:
        break;
    }

    return leftEntry.bin_name < rightEntry.bin_name;
}

FlirtWidget::FlirtWidget(MainWindow *main)
    : CutterDockWidget(main),
      ui(new Ui::FlirtWidget),
      blockMenu(new FlirtContextMenu(this, mainWindow))
{
    ui->setupUi(this);

    model = new FlirtModel(&sigdb, this);
    proxyModel = new FlirtProxyModel(model, this);
    ui->flirtTreeView->setModel(proxyModel);
    ui->flirtTreeView->sortByColumn(FlirtModel::BinTypeColumn, Qt::AscendingOrder);

    setScrollMode();

    this->connect(this, &QWidget::customContextMenuRequested, this,
                  &FlirtWidget::showItemContextMenu);
    this->setContextMenuPolicy(Qt::CustomContextMenu);

    this->connect(ui->flirtTreeView->selectionModel(), &QItemSelectionModel::currentChanged, this,
                  &FlirtWidget::onSelectedItemChanged);
    connect(Core(), &CutterCore::refreshAll, this, &FlirtWidget::refreshFlirt);

    this->addActions(this->blockMenu->actions());
}

FlirtWidget::~FlirtWidget() {}

void FlirtWidget::refreshFlirt()
{
    model->beginResetModel();
    sigdb = Core()->getSignaturesDB();
    model->endResetModel();

    ui->flirtTreeView->resizeColumnToContents(0);
    ui->flirtTreeView->resizeColumnToContents(1);
    ui->flirtTreeView->resizeColumnToContents(2);
    ui->flirtTreeView->resizeColumnToContents(3);
    ui->flirtTreeView->resizeColumnToContents(4);
    ui->flirtTreeView->resizeColumnToContents(5);
}

void FlirtWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->flirtTreeView);
}

void FlirtWidget::onSelectedItemChanged(const QModelIndex &index)
{
    if (index.isValid()) {
        const FlirtDescription &entry = sigdb.at(index.row());
        blockMenu->setTarget(entry);
    } else {
        blockMenu->clearTarget();
    }
}

void FlirtWidget::showItemContextMenu(const QPoint &pt)
{
    auto index = ui->flirtTreeView->currentIndex();
    if (index.isValid()) {
        const FlirtDescription &entry = sigdb.at(index.row());
        blockMenu->setTarget(entry);
        blockMenu->exec(this->mapToGlobal(pt));
    }
}