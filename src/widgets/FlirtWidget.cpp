#include "FlirtWidget.h"

#include "common/Helpers.h"
#include "core/MainWindow.h"
#include "ui_FlirtWidget.h"

FlirtModel::FlirtModel(QObject *parent) : QAbstractListModel(parent) {}

int FlirtModel::rowCount(const QModelIndex &) const
{
    return sigdb.count();
}

int FlirtModel::columnCount(const QModelIndex &) const
{
    return FlirtModel::ColumnCount;
}

QVariant FlirtModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= sigdb.count()) {
        return QVariant();
    }

    const FlirtDescription &entry = sigdb.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case BinTypeColumn:
            return entry.binName;
        case ArchNameColumn:
            return entry.archName;
        case ArchBitsColumn:
            return entry.archBits;
        case NumModulesColumn:
            return entry.nModules;
        case NameColumn:
            return entry.baseName;
        case DetailsColumn:
            return entry.details;
        default:
            return QVariant();
        }

    case FlirtDescriptionRole:
        return QVariant::fromValue(entry);

    case Qt::ToolTipRole: {
        return entry.shortPath;
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
    const QModelIndex index = sourceModel()->index(row, 0, parent);
    const auto entry = index.data(FlirtModel::FlirtDescriptionRole).value<FlirtDescription>();
    return qhelpers::filterStringContains(entry.baseName, this);
}

bool FlirtProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    const auto leftEntry = left.data(FlirtModel::FlirtDescriptionRole).value<FlirtDescription>();
    const auto rightEntry = right.data(FlirtModel::FlirtDescriptionRole).value<FlirtDescription>();

    switch (left.column()) {
    case FlirtModel::BinTypeColumn:
        return leftEntry.binName < rightEntry.binName;
    case FlirtModel::ArchNameColumn:
        return leftEntry.archName < rightEntry.archName;
    case FlirtModel::ArchBitsColumn:
        return leftEntry.archBits.toULongLong() < rightEntry.archBits.toULongLong();
    case FlirtModel::NumModulesColumn:
        return leftEntry.nModules.toULongLong() < rightEntry.nModules.toULongLong();
    case FlirtModel::NameColumn:
        return leftEntry.baseName < rightEntry.baseName;
    case FlirtModel::DetailsColumn:
        return leftEntry.details < rightEntry.details;
    default:
        break;
    }

    return leftEntry.binName < rightEntry.binName;
}

FlirtWidget::FlirtWidget(MainWindow *main)
    : CutterDockWidget(main),
      ui(new Ui::FlirtWidget),
      model(new FlirtModel(this)),
      proxyModel(new FlirtProxyModel(model, this)),
      blockMenu(new FlirtContextMenu(this, mainWindow))
{
    ui->setupUi(this);

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
    model->sigdb = Core()->getSignaturesDB();
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
        const FlirtDescription &entry = model->sigdb.at(index.row());
        blockMenu->setTarget(entry);
    } else {
        blockMenu->clearTarget();
    }
}

void FlirtWidget::showItemContextMenu(const QPoint &pt)
{
    auto index = ui->flirtTreeView->currentIndex();
    if (index.isValid()) {
        const FlirtDescription &entry = model->sigdb.at(index.row());
        blockMenu->setTarget(entry);
        blockMenu->exec(this->mapToGlobal(pt));
    }
}
