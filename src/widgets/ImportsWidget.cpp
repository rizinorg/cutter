#include "ImportsWidget.h"
#include "ui_ImportsWidget.h"

#include "MainWindow.h"
#include "common/Helpers.h"

#include "WidgetShortcuts.h"

#include <QTreeWidget>
#include <QPen>
#include <QPainter>

ImportsModel::ImportsModel(QList<ImportDescription> *imports, QObject *parent) :
    QAbstractTableModel(parent),
    imports(imports)
{}

int ImportsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : imports->count();
}

int ImportsModel::columnCount(const QModelIndex &) const
{
    return ImportsModel::ColumnCount;
}

QVariant ImportsModel::data(const QModelIndex &index, int role) const
{
    const ImportDescription &import = imports->at(index.row());
    switch (role) {
    case Qt::ForegroundRole:
        if (index.column() < ImportsModel::ColumnCount) {
            // Red color for unsafe functions
            if (banned.match(import.name).hasMatch())
                return Config()->getColor("gui.item_unsafe");
            // Grey color for symbols at offset 0 which can only be filled at runtime
            if (import.plt == 0)
                return Config()->getColor("gui.item_invalid");
        }
        break;
    case Qt::DisplayRole:
        switch (index.column()) {
        case ImportsModel::AddressColumn:
            return RAddressString(import.plt);
        case ImportsModel::TypeColumn:
            return import.type;
        case ImportsModel::SafetyColumn:
            return banned.match(import.name).hasMatch() ? tr("Unsafe") : QStringLiteral("");
        case ImportsModel::NameColumn:
            return import.name;
        default:
            break;
        }
        break;
    case ImportsModel::ImportDescriptionRole:
        return QVariant::fromValue(import);
    case ImportsModel::AddressRole:
        return import.plt;
    default:
        break;
    }
    return QVariant();
}

QVariant ImportsModel::headerData(int section, Qt::Orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        switch (section) {
        case ImportsModel::AddressColumn:
            return tr("Address");
        case ImportsModel::TypeColumn:
            return tr("Type");
        case ImportsModel::SafetyColumn:
            return tr("Safety");
        case ImportsModel::NameColumn:
            return tr("Name");
        default:
            break;
        }
    }
    return QVariant();
}

ImportsProxyModel::ImportsProxyModel(ImportsModel *sourceModel, QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(sourceModel);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

bool ImportsProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    QModelIndex index = sourceModel()->index(row, 0, parent);
    auto import = index.data(ImportsModel::ImportDescriptionRole).value<ImportDescription>();

    return import.name.contains(filterRegExp());
}

bool ImportsProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if (!left.isValid() || !right.isValid())
        return false;

    if (left.parent().isValid() || right.parent().isValid())
        return false;

    auto leftImport = left.data(ImportsModel::ImportDescriptionRole).value<ImportDescription>();
    auto rightImport = right.data(ImportsModel::ImportDescriptionRole).value<ImportDescription>();

    switch (left.column()) {
    case ImportsModel::AddressColumn:
        return leftImport.plt < rightImport.plt;
    case ImportsModel::TypeColumn:
        return leftImport.type < rightImport.type;
    case ImportsModel::SafetyColumn:
        break;
    case ImportsModel::NameColumn:
        return leftImport.name < rightImport.name;
    default:
        break;
    }

    return false;
}

/*
 * Imports Widget
 */

ImportsWidget::ImportsWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::ImportsWidget),
    importsModel(new ImportsModel(&imports, this)),
    importsProxyModel(new ImportsProxyModel(importsModel, this)),
    tree(new CutterTreeWidget(this))
{
    ui->setupUi(this);

    // Add Status Bar footer
    tree->addStatusBar(ui->verticalLayout);

    ui->importsTreeView->setModel(importsProxyModel);
    ui->importsTreeView->sortByColumn(ImportsModel::NameColumn, Qt::AscendingOrder);

    QShortcut *toggle_shortcut = new QShortcut(widgetShortcuts["ImportsWidget"], main);
    connect(toggle_shortcut, &QShortcut::activated, this, [=] (){ 
            toggleDockWidget(true); 
            main->updateDockActionChecked(action);
            } );

    // Ctrl-F to show/hide the filter entry
    QShortcut *searchShortcut = new QShortcut(QKeySequence::Find, this);
    connect(searchShortcut, &QShortcut::activated, ui->quickFilterView, &QuickFilterView::showFilter);
    searchShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    // Esc to clear the filter entry
    QShortcut *clearShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(clearShortcut, &QShortcut::activated, ui->quickFilterView, &QuickFilterView::clearFilter);
    clearShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    connect(ui->quickFilterView, SIGNAL(filterTextChanged(const QString &)),
            importsProxyModel, SLOT(setFilterWildcard(const QString &)));
    connect(ui->quickFilterView, SIGNAL(filterClosed()), ui->importsTreeView, SLOT(setFocus()));

    connect(ui->quickFilterView, &QuickFilterView::filterTextChanged, this, [this] {
        tree->showItemsNumber(importsProxyModel->rowCount());
    });
    
    setScrollMode();

    connect(Core(), SIGNAL(refreshAll()), this, SLOT(refreshImports()));
}

ImportsWidget::~ImportsWidget() {}

void ImportsWidget::refreshImports()
{
    importsModel->beginResetModel();
    imports = Core()->getAllImports();
    importsModel->endResetModel();
    qhelpers::adjustColumns(ui->importsTreeView, 4, 0);

    tree->showItemsNumber(importsProxyModel->rowCount());
}

void ImportsWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->importsTreeView);
}

void ImportsWidget::on_importsTreeView_doubleClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    Core()->seek(index.data(ImportsModel::AddressRole).toLongLong());
}
