#include "TypesWidget.h"
#include "ui_TypesWidget.h"
#include "core/MainWindow.h"
#include "common/Helpers.h"
#include "dialogs/TypesInteractionDialog.h"
#include "dialogs/LinkTypeDialog.h"

#include <QMenu>
#include <QFileDialog>
#include <QShortcut>
#include <QIcon>

TypesModel::TypesModel(QList<TypeDescription> *types, QObject *parent)
    : QAbstractListModel(parent),
      types(types)
{
}

int TypesModel::rowCount(const QModelIndex &) const
{
    return types->count();
}

int TypesModel::columnCount(const QModelIndex &) const
{
    return Columns::COUNT;
}

QVariant TypesModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= types->count())
        return QVariant();

    const TypeDescription &exp = types->at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case TYPE:
            return exp.type;
        case SIZE:
            return exp.size ? exp.size : QVariant();
        case FORMAT:
            return exp.format;
        case CATEGORY:
            return exp.category;
        default:
            return QVariant();
        }
    case TypeDescriptionRole:
        return QVariant::fromValue(exp);
    default:
        return QVariant();
    }
}

QVariant TypesModel::headerData(int section, Qt::Orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case TYPE:
            return tr("Type / Name");
        case SIZE:
            return tr("Size");
        case FORMAT:
            return tr("Format");
        case CATEGORY:
            return tr("Category");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

bool TypesModel::removeRows(int row, int count, const QModelIndex &parent)
{
    Core()->cmdRaw("t-" + types->at(row).type);
    beginRemoveRows(parent, row, row + count - 1);
    while (count--) {
        types->removeAt(row);
    }
    endRemoveRows();
    return true;
}


TypesSortFilterProxyModel::TypesSortFilterProxyModel(TypesModel *source_model, QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(source_model);
}

bool TypesSortFilterProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    QModelIndex index = sourceModel()->index(row, 0, parent);
    TypeDescription exp = index.data(TypesModel::TypeDescriptionRole).value<TypeDescription>();
    if (selectedCategory.isEmpty()) {
        return exp.type.contains(filterRegExp());
    } else {
        return selectedCategory == exp.category && exp.type.contains(filterRegExp());
    }
}

bool TypesSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    TypeDescription left_exp = left.data(TypesModel::TypeDescriptionRole).value<TypeDescription>();
    TypeDescription right_exp = right.data(TypesModel::TypeDescriptionRole).value<TypeDescription>();

    switch (left.column()) {
    case TypesModel::TYPE:
        return left_exp.type < right_exp.type;
    case TypesModel::SIZE:
        return left_exp.size < right_exp.size;
    case TypesModel::FORMAT:
        return left_exp.format < right_exp.format;
    case TypesModel::CATEGORY:
        return left_exp.category < right_exp.category;
    default:
        break;
    }

    return left_exp.size < right_exp.size;
}



TypesWidget::TypesWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::TypesWidget),
    tree(new CutterTreeWidget(this))
{
    ui->setupUi(this);
    ui->quickFilterView->setLabelText(tr("Category"));

    // Add status bar which displays the count
    tree->addStatusBar(ui->verticalLayout);

    // Set single select mode
    ui->typesTreeView->setSelectionMode(QAbstractItemView::SingleSelection);

    // Setup up the model and the proxy model
    types_model = new TypesModel(&types, this);
    types_proxy_model = new TypesSortFilterProxyModel(types_model, this);
    ui->typesTreeView->setModel(types_proxy_model);
    ui->typesTreeView->sortByColumn(TypesModel::TYPE, Qt::AscendingOrder);

    setScrollMode();

    // Setup custom context menu
    connect(ui->typesTreeView, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showTypesContextMenu(const QPoint &)));

    ui->typesTreeView->setContextMenuPolicy(Qt::CustomContextMenu);


    connect(ui->quickFilterView, SIGNAL(filterTextChanged(const QString &)), types_proxy_model,
            SLOT(setFilterWildcard(const QString &)));

    connect(ui->quickFilterView, &ComboQuickFilterView::filterTextChanged, this, [this] {
        tree->showItemsNumber(types_proxy_model->rowCount());
    });

    QShortcut *searchShortcut = new QShortcut(QKeySequence::Find, this);
    connect(searchShortcut, &QShortcut::activated, ui->quickFilterView, &ComboQuickFilterView::showFilter);
    searchShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    QShortcut *clearShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(clearShortcut, &QShortcut::activated, ui->quickFilterView, &ComboQuickFilterView::clearFilter);
    clearShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    connect(Core(), SIGNAL(refreshAll()), this, SLOT(refreshTypes()));

    connect(
        ui->quickFilterView->comboBox(), &QComboBox::currentTextChanged, this,
        [this]() {
            types_proxy_model->selectedCategory = ui->quickFilterView->comboBox()->currentData().toString();
            types_proxy_model->setFilterRegExp(types_proxy_model->filterRegExp());
            tree->showItemsNumber(types_proxy_model->rowCount());
        }
    );

    actionViewType = new QAction(tr("View Type"), this);
    actionEditType = new QAction(tr("Edit Type"), this);

    connect (actionViewType, &QAction::triggered, [this]() { viewType(true) ;});
    connect (actionEditType, &QAction::triggered, [this]() { viewType(false) ;});
    connect (ui->typesTreeView, &QTreeView::doubleClicked, this, &TypesWidget::typeItemDoubleClicked);
}

TypesWidget::~TypesWidget() {}

void TypesWidget::refreshTypes()
{
    types_model->beginResetModel();
    types = Core()->getAllTypes();
    types_model->endResetModel();

    QStringList categories;
    for (TypeDescription exp: types) {
        categories << exp.category;
    }
    categories.removeDuplicates();
    refreshCategoryCombo(categories);

    qhelpers::adjustColumns(ui->typesTreeView, 4, 0);
}

void TypesWidget::refreshCategoryCombo(const QStringList &categories)
{
    QComboBox *combo = ui->quickFilterView->comboBox();

    combo->clear();
    combo->addItem(tr("(All)"));

    for (const QString &category : categories) {
        combo->addItem(category, category);
    }

    types_proxy_model->selectedCategory.clear();
}

void TypesWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->typesTreeView);
}

void TypesWidget::showTypesContextMenu(const QPoint &pt)
{
    QModelIndex index = ui->typesTreeView->indexAt(pt);

    QMenu menu(ui->typesTreeView);
    menu.addAction(ui->actionLoad_New_Types);

    if (index.isValid()) {
        TypeDescription t = index.data(TypesModel::TypeDescriptionRole).value<TypeDescription>();
        if (t.category != "Primitive") {
            // Add "Link To Address" option
            menu.addAction(actionViewType);
            menu.addAction(actionEditType);
            if (t.category == "Struct") {
                menu.addAction(ui->actionLink_Type_To_Address);
            }
        }
    }

    menu.addAction(ui->actionExport_Types);

    if (index.isValid()) {
        TypeDescription t = index.data(TypesModel::TypeDescriptionRole).value<TypeDescription>();
        if (t.category != "Typedef") {
            menu.addSeparator();
            menu.addAction(ui->actionDelete_Type);
        }
    }

    menu.exec(ui->typesTreeView->mapToGlobal(pt));
}

void TypesWidget::on_actionExport_Types_triggered()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Save File"), Config()->getRecentFolder());
    if (filename.isEmpty()) {
        return;
    }
    Config()->setRecentFolder(QFileInfo(filename).absolutePath());
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, tr("Error"), file.errorString());
        on_actionExport_Types_triggered();
        return;
    }
    QTextStream fileOut(&file);
    fileOut << Core()->cmd("tc");
    file.close();
}

void TypesWidget::on_actionLoad_New_Types_triggered()
{
    TypesInteractionDialog dialog(this);
    connect(&dialog, &TypesInteractionDialog::newTypesLoaded, this, &TypesWidget::refreshTypes);
    dialog.setWindowTitle(tr("Load New Types"));
    dialog.exec();
}

void TypesWidget::viewType(bool readOnly)
{

    QModelIndex index = ui->typesTreeView->currentIndex();

    if (!index.isValid()) {
        return;
    }

    TypesInteractionDialog dialog(this, readOnly);
    TypeDescription t = index.data(TypesModel::TypeDescriptionRole).value<TypeDescription>();
    if (!readOnly) {
        dialog.setWindowTitle(tr("Edit Type: ") + t.type);
    connect(&dialog, &TypesInteractionDialog::newTypesLoaded, this, &TypesWidget::refreshTypes);
    } else {
        dialog.setWindowTitle(tr("View Type: ") + t.type + tr(" (Read Only)"));
    }
    dialog.fillTextArea(Core()->getTypeAsC(t.type, t.category));
    dialog.exec();
}


void TypesWidget::on_actionDelete_Type_triggered()
{
    QModelIndex proxyIndex = ui->typesTreeView->currentIndex();
    if (!proxyIndex.isValid()) {
        return;
    }
    QModelIndex index = types_proxy_model->mapToSource(proxyIndex);
    if (!index.isValid()) {
        return;
    }

    TypeDescription exp = index.data(TypesModel::TypeDescriptionRole).value<TypeDescription>();
    QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Cutter"), tr("Are you sure you want to delete \"%1\"?").arg(exp.type));
    if (reply == QMessageBox::Yes) {
        types_model->removeRow(index.row());
    }
}

void TypesWidget::on_actionLink_Type_To_Address_triggered()
{
    LinkTypeDialog dialog(this);

    QModelIndex index = ui->typesTreeView->currentIndex();
    if (index.isValid()) {
        TypeDescription t = index.data(TypesModel::TypeDescriptionRole).value<TypeDescription>();
        dialog.setDefaultType(t.type);
        dialog.setDefaultAddress(RAddressString(Core()->getOffset()));
        dialog.exec();
    }
}

void TypesWidget::typeItemDoubleClicked(const QModelIndex &index) {
    if (!index.isValid()) {
        return;
    }

    TypesInteractionDialog dialog(this, true);
    TypeDescription t = index.data(TypesModel::TypeDescriptionRole).value<TypeDescription>();
    if (t.category == "Primitive") {
        return;
    }
    dialog.fillTextArea(Core()->getTypeAsC(t.type, t.category));
    dialog.setWindowTitle(tr("View Type: ") + t.type + tr(" (Read Only)"));
    dialog.exec();
}
