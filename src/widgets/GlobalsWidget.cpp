#include "GlobalsWidget.h"
#include "ui_GlobalsWidget.h"
#include "core/MainWindow.h"
#include "common/Helpers.h"
#include "dialogs/GlobalVariableDialog.h"
#include "shortcuts/ShortcutManager.h"

#include <QMenu>
#include <QShortcut>

GlobalsModel::GlobalsModel(QList<GlobalDescription> *globals, QObject *parent)
    : AddressableItemModel<QAbstractListModel>(parent), globals(globals)
{
}

int GlobalsModel::rowCount(const QModelIndex &) const
{
    return globals->count();
}

int GlobalsModel::columnCount(const QModelIndex &) const
{
    return GlobalsModel::ColumnCount;
}

QVariant GlobalsModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= globals->count()) {
        return QVariant();
    }

    const GlobalDescription &global = globals->at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case GlobalsModel::AddressColumn:
            return RzAddressString(global.addr);
        case GlobalsModel::TypeColumn:
            return QString(global.type).trimmed();
        case GlobalsModel::NameColumn:
            return global.name;
        case GlobalsModel::CommentColumn:
            return Core()->getCommentAt(global.addr);
        default:
            return QVariant();
        }
    case GlobalsModel::GlobalDescriptionRole:
        return QVariant::fromValue(global);
    default:
        return QVariant();
    }
}

QVariant GlobalsModel::headerData(int section, Qt::Orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case GlobalsModel::AddressColumn:
            return tr("Address");
        case GlobalsModel::TypeColumn:
            return tr("Type");
        case GlobalsModel::NameColumn:
            return tr("Name");
        case GlobalsModel::CommentColumn:
            return tr("Comment");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

RVA GlobalsModel::address(const QModelIndex &index) const
{
    const GlobalDescription &global = globals->at(index.row());
    return global.addr;
}

QString GlobalsModel::name(const QModelIndex &index) const
{
    const GlobalDescription &global = globals->at(index.row());
    return global.name;
}

GlobalsProxyModel::GlobalsProxyModel(GlobalsModel *sourceModel, QObject *parent)
    : AddressableFilterProxyModel(sourceModel, parent)
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

bool GlobalsProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    QModelIndex index = sourceModel()->index(row, 0, parent);
    auto global = index.data(GlobalsModel::GlobalDescriptionRole).value<GlobalDescription>();

    return qhelpers::filterStringContains(global.name, this);
}

bool GlobalsProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    auto leftGlobal = left.data(GlobalsModel::GlobalDescriptionRole).value<GlobalDescription>();
    auto rightGlobal = right.data(GlobalsModel::GlobalDescriptionRole).value<GlobalDescription>();

    switch (left.column()) {
    case GlobalsModel::AddressColumn:
        return leftGlobal.addr < rightGlobal.addr;
    case GlobalsModel::TypeColumn:
        return leftGlobal.type < rightGlobal.type;
    case GlobalsModel::NameColumn:
        return leftGlobal.name < rightGlobal.name;
    case GlobalsModel::CommentColumn:
        return Core()->getCommentAt(leftGlobal.addr) < Core()->getCommentAt(rightGlobal.addr);
    default:
        break;
    }

    return false;
}

void GlobalsWidget::editGlobal()
{
    QModelIndex index = ui->treeView->currentIndex();

    if (!index.isValid()) {
        return;
    }

    RVA globalVariableAddress = globalsProxyModel->address(index);

    GlobalVariableDialog dialog(globalVariableAddress, parentWidget());
    dialog.exec();
}

void GlobalsWidget::deleteGlobal()
{
    QModelIndex index = ui->treeView->currentIndex();

    if (!index.isValid()) {
        return;
    }

    RVA globalVariableAddress = globalsProxyModel->address(index);
    Core()->delGlobalVariable(globalVariableAddress);
}

GlobalsWidget::GlobalsWidget(MainWindow *main)
    : CutterDockWidget(main), ui(new Ui::GlobalsWidget), tree(new CutterTreeWidget(this))
{
    ui->setupUi(this);
    ui->quickFilterView->setLabelText(tr("Category"));

    setWindowTitle(tr("Globals"));
    setObjectName("GlobalsWidget");

    // Add status bar which displays the count
    tree->addStatusBar(ui->verticalLayout);

    // Set single select mode
    ui->treeView->setSelectionMode(QAbstractItemView::SingleSelection);

    ui->treeView->setMainWindow(mainWindow);

    // Setup up the model and the proxy model
    globalsModel = new GlobalsModel(&globals, this);
    globalsProxyModel = new GlobalsProxyModel(globalsModel, this);
    ui->treeView->setModel(globalsProxyModel);
    ui->treeView->sortByColumn(GlobalsModel::AddressColumn, Qt::AscendingOrder);

    // Setup custom context menu
    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->quickFilterView, &ComboQuickFilterView::filterTextChanged, globalsProxyModel,
            &QSortFilterProxyModel::setFilterWildcard);

    connect(ui->quickFilterView, &ComboQuickFilterView::filterTextChanged, this,
            [this] { tree->showItemsNumber(globalsProxyModel->rowCount()); });

    QShortcut *searchShortcut = Shortcuts()->makeQShortcut("General.showFilter", this);
    connect(searchShortcut, &QShortcut::activated, ui->quickFilterView,
            &ComboQuickFilterView::showFilter);
    searchShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    QShortcut *clearShortcut = Shortcuts()->makeQShortcut("General.clearFilter", this);
    connect(clearShortcut, &QShortcut::activated, ui->quickFilterView,
            &ComboQuickFilterView::clearFilter);
    clearShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    actionEditGlobal = new QAction(tr("Edit Global Variable"), this);
    actionDeleteGlobal = new QAction(tr("Delete Global Variable"), this);

    auto menu = ui->treeView->getItemContextMenu();
    menu->addAction(actionEditGlobal);
    menu->addAction(actionDeleteGlobal);

    connect(actionEditGlobal, &QAction::triggered, this, [this]() { editGlobal(); });
    connect(actionDeleteGlobal, &QAction::triggered, this, [this]() { deleteGlobal(); });

    connect(Core(), &CutterCore::globalVarsChanged, this, &GlobalsWidget::refreshGlobals);
    connect(Core(), &CutterCore::codeRebased, this, &GlobalsWidget::refreshGlobals);
    connect(Core(), &CutterCore::refreshAll, this, &GlobalsWidget::refreshGlobals);
    connect(Core(), &CutterCore::commentsChanged, this,
            [this]() { qhelpers::emitColumnChanged(globalsModel, GlobalsModel::CommentColumn); });
}

GlobalsWidget::~GlobalsWidget() {}

void GlobalsWidget::refreshGlobals()
{
    globalsModel->beginResetModel();
    globals = Core()->getAllGlobals();
    globalsModel->endResetModel();

    qhelpers::adjustColumns(ui->treeView, GlobalsModel::ColumnCount, 0);
}
