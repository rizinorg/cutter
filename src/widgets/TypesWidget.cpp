#include "TypesWidget.h"

#include "common/CutterSearchable.h"
#include "common/Helpers.h"
#include "core/MainWindow.h"
#include "dialogs/TypesInteractionDialog.h"
#include "dialogs/TypesVariablesDialog.h"
#include "shortcuts/ShortcutManager.h"
#include "ui_TypesWidget.h"

#include <QFileDialog>
#include <QIcon>
#include <QMenu>
#include <QShortcut>

#include <utility>

TypesModel::TypesModel(QObject *parent) : QAbstractListModel(parent) {}

QVariant TypesModel::toolTipValue(const QModelIndex &index) const
{
    const auto t = index.data(TypesModel::typeDescriptionRole).value<TypeDescription>();

    if (t.category == "Primitive") {
        return QVariant();
    }

    return Core()->getTypeAsC(t.type).trimmed();
}

int TypesModel::rowCount(const QModelIndex &) const
{
    return types.count();
}

int TypesModel::columnCount(const QModelIndex &) const
{
    return Columns::COUNT;
}

QVariant TypesModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= types.count()) {
        return QVariant();
    }

    const TypeDescription &exp = types.at(index.row());

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
    case Qt::ToolTipRole:
        return toolTipValue(index);
    case typeDescriptionRole:
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
    RzCoreLocked core(Core());
    rz_type_db_del(rz_analysis_get_type_db(core->analysis),
                   types.at(row).type.toUtf8().constData());
    beginRemoveRows(parent, row, row + count - 1);
    while (count--) {
        types.removeAt(row);
    }
    endRemoveRows();
    return true;
}

TypesSortFilterProxyModel::TypesSortFilterProxyModel(TypesModel *source_model, QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(source_model);
}

void TypesSortFilterProxyModel::setCategory(QString category)
{
    beginResetModel();
    selectedCategory = std::move(category);
    endResetModel();
}

bool TypesSortFilterProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    const QModelIndex index = sourceModel()->index(row, 0, parent);
    const auto exp = index.data(TypesModel::typeDescriptionRole).value<TypeDescription>();
    if (!selectedCategory.isEmpty() && selectedCategory != exp.category) {
        return false;
    }
    return qhelpers::filterStringContains(exp.type, this);
}

bool TypesSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    const auto leftExp = left.data(TypesModel::typeDescriptionRole).value<TypeDescription>();
    const auto rightExp = right.data(TypesModel::typeDescriptionRole).value<TypeDescription>();

    switch (left.column()) {
    case TypesModel::TYPE:
        return leftExp.type < rightExp.type;
    case TypesModel::SIZE:
        return leftExp.size < rightExp.size;
    case TypesModel::FORMAT:
        return leftExp.format < rightExp.format;
    case TypesModel::CATEGORY:
        return leftExp.category < rightExp.category;
    default:
        break;
    }

    return leftExp.size < rightExp.size;
}

TypesWidget::TypesWidget(MainWindow *main)
    : CutterDockWidget(main),
      ui(new Ui::TypesWidget),
      typesModel(new TypesModel(this)),
      typesProxyModel(new TypesSortFilterProxyModel(typesModel, this))
{
    ui->setupUi(this);
    ui->quickFilterView->setLabelText(tr("Category"));

    // Set single select mode
    ui->typesTreeView->setSelectionMode(QAbstractItemView::SingleSelection);

    // Setup up the model and the proxy model

    ui->typesTreeView->setModel(typesProxyModel);
    ui->typesTreeView->sortByColumn(TypesModel::TYPE, Qt::AscendingOrder);

    setScrollMode();

    // Setup custom context menu
    connect(ui->typesTreeView, &QWidget::customContextMenuRequested, this,
            &TypesWidget::showTypesContextMenu);

    ui->typesTreeView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->quickFilterView, &ComboQuickFilterView::filterChanged, this,
            [this](const QString &text, int options) {
                qhelpers::applyFilter(typesProxyModel, text, options);
            });

    connect(ui->quickFilterView, &ComboQuickFilterView::filterTextChanged, this,
            [this] { ui->quickFilterView->setItemCount(typesProxyModel->rowCount()); });

    QShortcut *searchShortcut = Shortcuts()->makeQShortcut("General.showFilter", this);
    connect(searchShortcut, &QShortcut::activated, ui->quickFilterView,
            &ComboQuickFilterView::showFilter);
    searchShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    QShortcut *clearShortcut = Shortcuts()->makeQShortcut("General.clearFilter", this);
    connect(clearShortcut, &QShortcut::activated, ui->quickFilterView,
            &ComboQuickFilterView::clearFilter);
    clearShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    connect(Core(), &CutterCore::refreshAll, this, &TypesWidget::refreshTypes);

    connect(ui->quickFilterView->comboBox(), &QComboBox::currentTextChanged, this, [this]() {
        typesProxyModel->setCategory(ui->quickFilterView->comboBox()->currentData().toString());
        ui->quickFilterView->setItemCount(typesProxyModel->rowCount());
    });

    actionViewType = new QAction(tr("View Type"), this);
    actionEditType = new QAction(tr("Edit Type"), this);
    actionShowVariables = new QAction(tr("Show Variables and Globals of this Type"), this);

    connect(actionViewType, &QAction::triggered, [this]() { viewType(true); });
    connect(actionEditType, &QAction::triggered, [this]() { viewType(false); });
    connect(ui->typesTreeView, &QTreeView::doubleClicked, this,
            &TypesWidget::typeItemDoubleClicked);

    connect(actionShowVariables, &QAction::triggered, [this]() { showVariables(); });

    connect(ui->actionExportTypes, &QAction::triggered, this,
            &TypesWidget::onActionExportTypesTriggered);
    connect(ui->actionLoadNewTypes, &QAction::triggered, this,
            &TypesWidget::onActionLoadNewTypesTriggered);
    connect(ui->actionDeleteType, &QAction::triggered, this,
            &TypesWidget::onActionDeleteTypeTriggered);
}

TypesWidget::~TypesWidget() {}

void TypesWidget::refreshTypes()
{
    typesModel->beginResetModel();
    typesModel->types = Core()->getAllTypes();
    typesModel->endResetModel();

    QStringList categories;
    for (const TypeDescription &exp : std::as_const(typesModel->types)) {
        categories << exp.category;
    }
    categories.removeDuplicates();
    refreshCategoryCombo(categories);

    // set the initial count
    ui->quickFilterView->setItemCount(typesProxyModel->rowCount());

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

    typesProxyModel->setCategory(QString());
}

void TypesWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->typesTreeView);
}

void TypesWidget::showTypesContextMenu(const QPoint &pt)
{
    const QModelIndex index = ui->typesTreeView->indexAt(pt);

    QMenu menu(ui->typesTreeView);
    menu.addAction(ui->actionLoadNewTypes);

    if (index.isValid()) {
        const auto t = index.data(TypesModel::typeDescriptionRole).value<TypeDescription>();
        if (t.category != "Primitive") {
            // Add "Link To Address" option
            menu.addAction(actionViewType);
            menu.addAction(actionEditType);
            menu.addAction(actionShowVariables);
        }
    }

    menu.addAction(ui->actionExportTypes);

    if (index.isValid()) {
        const auto t = index.data(TypesModel::typeDescriptionRole).value<TypeDescription>();
        if (t.category != "Typedef") {
            menu.addSeparator();
            menu.addAction(ui->actionDeleteType);
        }
    }

    menu.exec(ui->typesTreeView->mapToGlobal(pt));
}

void TypesWidget::onActionExportTypesTriggered()
{
    auto core = Core()->lock();
    char *str = rz_core_types_as_c_all(core, true);
    if (!str) {
        return;
    }
    const QString filename =
            QFileDialog::getSaveFileName(this, tr("Save File"), Config()->getRecentFolder());
    if (filename.isEmpty()) {
        return;
    }
    Config()->setRecentFolder(QFileInfo(filename).absolutePath());
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, tr("Error"), file.errorString());
        onActionExportTypesTriggered();
        return;
    }
    QTextStream fileOut(&file);
    fileOut << str;
    free(str);
    file.close();
}

void TypesWidget::onActionLoadNewTypesTriggered()
{
    const QModelIndex index = ui->typesTreeView->currentIndex();
    if (!index.isValid()) {
        return;
    }

    const auto t = index.data(TypesModel::typeDescriptionRole).value<TypeDescription>();

    TypesInteractionDialog dialog(this);
    connect(&dialog, &TypesInteractionDialog::newTypesLoaded, this, &TypesWidget::refreshTypes);
    dialog.setWindowTitle(tr("Load New Types"));
    dialog.exec();
}

void TypesWidget::viewType(bool readOnly)
{

    const QModelIndex index = ui->typesTreeView->currentIndex();

    if (!index.isValid()) {
        return;
    }

    TypesInteractionDialog dialog(this, readOnly);
    const auto t = index.data(TypesModel::typeDescriptionRole).value<TypeDescription>();
    if (!readOnly) {
        dialog.setWindowTitle(tr("Edit Type: ") + t.type);
        connect(&dialog, &TypesInteractionDialog::newTypesLoaded, this, &TypesWidget::refreshTypes);
    } else {
        dialog.setWindowTitle(tr("View Type: ") + t.type + tr(" (Read Only)"));
    }
    dialog.fillTextArea(Core()->getTypeAsC(t.type));
    dialog.setTypeName(t.type);
    dialog.exec();
}

void TypesWidget::onActionDeleteTypeTriggered()
{
    const QModelIndex proxyIndex = ui->typesTreeView->currentIndex();
    if (!proxyIndex.isValid()) {
        return;
    }
    const QModelIndex index = typesProxyModel->mapToSource(proxyIndex);
    if (!index.isValid()) {
        return;
    }

    const auto exp = index.data(TypesModel::typeDescriptionRole).value<TypeDescription>();
    const QMessageBox::StandardButton reply = QMessageBox::question(
            this, tr("Cutter"), tr("Are you sure you want to delete \"%1\"?").arg(exp.type));
    if (reply == QMessageBox::Yes) {
        typesModel->removeRow(index.row());
    }
}

void TypesWidget::typeItemDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    TypesInteractionDialog dialog(this, true);
    const auto t = index.data(TypesModel::typeDescriptionRole).value<TypeDescription>();
    if (t.category == "Primitive") {
        return;
    }
    dialog.fillTextArea(Core()->getTypeAsC(t.type));
    dialog.setWindowTitle(tr("View Type: ") + t.type + tr(" (Read Only)"));
    dialog.setTypeName(t.type);
    dialog.exec();
}

void TypesWidget::showVariables()
{
    const QModelIndex index = ui->typesTreeView->currentIndex();

    if (!index.isValid()) {
        return;
    }
    const auto t = index.data(TypesModel::typeDescriptionRole).value<TypeDescription>();

    TypesVariablesDialog tvd(this, t.type);
    tvd.exec();
}

void TypesWidget::selectTypeByName(const QString &typeName)
{
    if (typeName.isEmpty()) {
        return;
    }

    QModelIndexList results = typesProxyModel->match(typesProxyModel->index(0, 0), Qt::DisplayRole,
                                                     typeName, 1, Qt::MatchExactly);

    // if results are empty, remove the filter and try again
    // avoids removing the filter unnecessarily
    bool isTextFilterEmpty;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    isTextFilterEmpty = typesProxyModel->filterRegExp().pattern().isEmpty();
#else
    isTextFilterEmpty = typesProxyModel->filterRegularExpression().pattern().isEmpty();
#endif
    if (results.isEmpty()
        && (!isTextFilterEmpty || ui->quickFilterView->comboBox()->currentIndex() != 0)) {

        ui->quickFilterView->clearFilter();
        ui->quickFilterView->comboBox()->setCurrentIndex(0); // select (All)
        typesProxyModel->setFilterFixedString("");
        results = typesProxyModel->match(typesProxyModel->index(0, 0), Qt::DisplayRole, typeName, 1,
                                         Qt::MatchExactly);
    }

    if (results.isEmpty()) {
        return;
    }

    const QModelIndex index = results.first();
    ui->typesTreeView->setCurrentIndex(index);
    ui->typesTreeView->selectionModel()->select(
            index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    ui->typesTreeView->scrollTo(index, QAbstractItemView::PositionAtCenter);
    ui->typesTreeView->setFocus();
}
