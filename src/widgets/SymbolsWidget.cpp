#include "SymbolsWidget.h"
#include "ui_ListDockWidget.h"
#include "core/MainWindow.h"
#include "common/Helpers.h"

#include <QShortcut>

SymbolsModel::SymbolsModel(QList<SymbolDescription> *symbols, QObject *parent)
    : AddressableItemModel<QAbstractListModel>(parent),
      symbols(symbols)
{
}

int SymbolsModel::rowCount(const QModelIndex &) const
{
    return symbols->count();
}

int SymbolsModel::columnCount(const QModelIndex &) const
{
    return SymbolsModel::ColumnCount;
}

QVariant SymbolsModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= symbols->count()) {
        return QVariant();
    }

    const SymbolDescription &symbol = symbols->at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case SymbolsModel::AddressColumn:
            return RAddressString(symbol.vaddr);
        case SymbolsModel::TypeColumn:
            return QString("%1 %2").arg(symbol.bind, symbol.type).trimmed();
        case SymbolsModel::NameColumn:
            return symbol.name;
        default:
            return QVariant();
        }
    case SymbolsModel::SymbolDescriptionRole:
        return QVariant::fromValue(symbol);
    default:
        return QVariant();
    }
}

QVariant SymbolsModel::headerData(int section, Qt::Orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case SymbolsModel::AddressColumn:
            return tr("Address");
        case SymbolsModel::TypeColumn:
            return tr("Type");
        case SymbolsModel::NameColumn:
            return tr("Name");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

RVA SymbolsModel::address(const QModelIndex &index) const
{
    const SymbolDescription &symbol = symbols->at(index.row());
    return symbol.vaddr;
}

QString SymbolsModel::name(const QModelIndex &index) const
{
    const SymbolDescription &symbol = symbols->at(index.row());
    return symbol.name;
}

SymbolsProxyModel::SymbolsProxyModel(SymbolsModel *sourceModel, QObject *parent)
    : AddressableFilterProxyModel(sourceModel, parent)
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

bool SymbolsProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    QModelIndex index = sourceModel()->index(row, 0, parent);
    auto symbol = index.data(SymbolsModel::SymbolDescriptionRole).value<SymbolDescription>();

    return symbol.name.contains(filterRegExp());
}

bool SymbolsProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    auto leftSymbol = left.data(SymbolsModel::SymbolDescriptionRole).value<SymbolDescription>();
    auto rightSymbol = right.data(SymbolsModel::SymbolDescriptionRole).value<SymbolDescription>();

    switch (left.column()) {
    case SymbolsModel::AddressColumn:
        return leftSymbol.vaddr < rightSymbol.vaddr;
    case SymbolsModel::TypeColumn:
        return leftSymbol.type < rightSymbol.type;
    case SymbolsModel::NameColumn:
        return leftSymbol.name < rightSymbol.name;
    default:
        break;
    }

    return false;
}

SymbolsWidget::SymbolsWidget(MainWindow *main) :
    ListDockWidget(main)
{
    setWindowTitle(tr("Symbols"));
    setObjectName("SymbolsWidget");

    symbolsModel = new SymbolsModel(&symbols, this);
    symbolsProxyModel = new SymbolsProxyModel(symbolsModel, this);
    setModels(symbolsProxyModel);
    ui->treeView->sortByColumn(SymbolsModel::AddressColumn, Qt::AscendingOrder);

    connect(Core(), &CutterCore::codeRebased, this, &SymbolsWidget::refreshSymbols);
    connect(Core(), &CutterCore::refreshAll, this, &SymbolsWidget::refreshSymbols);
}

SymbolsWidget::~SymbolsWidget() {}

void SymbolsWidget::refreshSymbols()
{
    symbolsModel->beginResetModel();
    symbols = Core()->getAllSymbols();
    symbolsModel->endResetModel();

    qhelpers::adjustColumns(ui->treeView, SymbolsModel::ColumnCount, 0);
}
