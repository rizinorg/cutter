#include "ExportsWidget.h"
#include "ui_ExportsWidget.h"
#include "core/MainWindow.h"
#include "common/Helpers.h"
#include "WidgetShortcuts.h"

#include <QShortcut>

ExportsModel::ExportsModel(QList<ExportDescription> *exports, QObject *parent)
    : QAbstractListModel(parent),
      exports(exports)
{
}

int ExportsModel::rowCount(const QModelIndex &) const
{
    return exports->count();
}

int ExportsModel::columnCount(const QModelIndex &) const
{
    return ExportsModel::ColumnCount;
}

QVariant ExportsModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= exports->count())
        return QVariant();

    const ExportDescription &exp = exports->at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case ExportsModel::OffsetColumn:
            return RAddressString(exp.vaddr);
        case ExportsModel::SizeColumn:
            return RSizeString(exp.size);
        case ExportsModel::TypeColumn:
            return exp.type;
        case ExportsModel::NameColumn:
            return exp.name;
        default:
            return QVariant();
        }
    case ExportsModel::ExportDescriptionRole:
        return QVariant::fromValue(exp);
    default:
        return QVariant();
    }
}

QVariant ExportsModel::headerData(int section, Qt::Orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case ExportsModel::OffsetColumn:
            return tr("Address");
        case ExportsModel::SizeColumn:
            return tr("Size");
        case ExportsModel::TypeColumn:
            return tr("Type");
        case ExportsModel::NameColumn:
            return tr("Name");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

ExportsProxyModel::ExportsProxyModel(ExportsModel *source_model, QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(source_model);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

bool ExportsProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    QModelIndex index = sourceModel()->index(row, 0, parent);
    auto exp = index.data(ExportsModel::ExportDescriptionRole).value<ExportDescription>();

    return exp.name.contains(filterRegExp());
}

bool ExportsProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    auto leftExp = left.data(ExportsModel::ExportDescriptionRole).value<ExportDescription>();
    auto rightExp = right.data(ExportsModel::ExportDescriptionRole).value<ExportDescription>();

    switch (left.column()) {
    case ExportsModel::SizeColumn:
        if (leftExp.size != rightExp.size)
            return leftExp.size < rightExp.size;
    // fallthrough
    case ExportsModel::OffsetColumn:
        if (leftExp.vaddr != rightExp.vaddr)
            return leftExp.vaddr < rightExp.vaddr;
    // fallthrough
    case ExportsModel::NameColumn:
        return leftExp.name < rightExp.name;
    case ExportsModel::TypeColumn:
        if (leftExp.type != rightExp.type)
            return leftExp.type < rightExp.type;
    default:
        break;
    }

    // fallback
    return leftExp.vaddr < rightExp.vaddr;
}

ExportsWidget::ExportsWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::ExportsWidget),
    tree(new CutterTreeWidget(this))
{
    ui->setupUi(this);

    // Add Status Bar footer
    tree->addStatusBar(ui->verticalLayout);
    
    exportsModel = new ExportsModel(&exports, this);
    exportsProxyModel = new ExportsProxyModel(exportsModel, this);
    ui->exportsTreeView->setModel(exportsProxyModel);
    ui->exportsTreeView->sortByColumn(ExportsModel::OffsetColumn, Qt::AscendingOrder);

    QShortcut *toggle_shortcut = new QShortcut(widgetShortcuts["ExportsWidget"], main);
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
            exportsProxyModel, SLOT(setFilterWildcard(const QString &)));
    connect(ui->quickFilterView, SIGNAL(filterClosed()), ui->exportsTreeView, SLOT(setFocus()));

    connect(ui->quickFilterView, &QuickFilterView::filterTextChanged, this, [this] {
        tree->showItemsNumber(exportsProxyModel->rowCount());
    });
    
    setScrollMode();

    connect(Core(), SIGNAL(refreshAll()), this, SLOT(refreshExports()));
}

ExportsWidget::~ExportsWidget() {}

void ExportsWidget::refreshExports()
{
    exportsModel->beginResetModel();
    exports = Core()->getAllExports();
    exportsModel->endResetModel();

    qhelpers::adjustColumns(ui->exportsTreeView, 3, 0);

    tree->showItemsNumber(exportsProxyModel->rowCount());
}


void ExportsWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->exportsTreeView);
}

void ExportsWidget::on_exportsTreeView_doubleClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    ExportDescription exp = index.data(ExportsModel::ExportDescriptionRole).value<ExportDescription>();
    Core()->seek(exp.vaddr);
}
