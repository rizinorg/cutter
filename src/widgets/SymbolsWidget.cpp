#include "SymbolsWidget.h"
#include "ui_SymbolsWidget.h"
#include "core/MainWindow.h"
#include "common/Helpers.h"

#include <QShortcut>

SymbolsModel::SymbolsModel(QList<SymbolDescription> *symbols, QObject *parent)
    : QAbstractListModel(parent),
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

SymbolsProxyModel::SymbolsProxyModel(SymbolsModel *sourceModel, QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(sourceModel);
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

SymbolsWidget::SymbolsWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::SymbolsWidget),
    tree(new CutterTreeWidget(this))
{
    ui->setupUi(this);

    // Add Status Bar footer
    tree->addStatusBar(ui->verticalLayout);

    symbolsModel = new SymbolsModel(&symbols, this);
    symbolsProxyModel = new SymbolsProxyModel(symbolsModel, this);
    ui->symbolsTreeView->setModel(symbolsProxyModel);
    ui->symbolsTreeView->sortByColumn(SymbolsModel::AddressColumn, Qt::AscendingOrder);

    // Ctrl-F to show/hide the filter entry
    QShortcut *searchShortcut = new QShortcut(QKeySequence::Find, this);
    connect(searchShortcut, &QShortcut::activated, ui->quickFilterView, &QuickFilterView::showFilter);
    searchShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    // Esc to clear the filter entry
    QShortcut *clearShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(clearShortcut, &QShortcut::activated, ui->quickFilterView, &QuickFilterView::clearFilter);
    clearShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    connect(ui->quickFilterView, SIGNAL(filterTextChanged(const QString &)),
            symbolsProxyModel, SLOT(setFilterWildcard(const QString &)));
    connect(ui->quickFilterView, SIGNAL(filterClosed()), ui->symbolsTreeView, SLOT(setFocus()));

    connect(ui->quickFilterView, &QuickFilterView::filterTextChanged, this, [this] {
        tree->showItemsNumber(symbolsProxyModel->rowCount());
    });
    
    setScrollMode();

    connect(Core(), SIGNAL(refreshAll()), this, SLOT(refreshSymbols()));
}

SymbolsWidget::~SymbolsWidget() {}


void SymbolsWidget::on_symbolsTreeView_doubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    auto symbol = index.data(SymbolsModel::SymbolDescriptionRole).value<SymbolDescription>();
    Core()->seek(symbol.vaddr);
}

void SymbolsWidget::refreshSymbols()
{
    symbolsModel->beginResetModel();
    symbols = Core()->getAllSymbols();
    symbolsModel->endResetModel();

    qhelpers::adjustColumns(ui->symbolsTreeView, SymbolsModel::ColumnCount, 0);

    tree->showItemsNumber(symbolsProxyModel->rowCount());
}

void SymbolsWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->symbolsTreeView);
}
