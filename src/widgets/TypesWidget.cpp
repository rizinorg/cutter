#include "TypesWidget.h"
#include "ui_TypesWidget.h"
#include "MainWindow.h"
#include "common/Helpers.h"

#include "dialogs/LoadNewTypesDialog.h"

#include <QMenu>
#include <QFileDialog>

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
            return exp.category == tr("Primitive") ? exp.size : QVariant();
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
            return tr("Type");
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

    qhelpers::adjustColumns(ui->typesTreeView, 3, 0);
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
    QMenu *menu = new QMenu(ui->typesTreeView);

    menu->clear();
    menu->addAction(ui->actionLoad_New_Types);
    menu->addAction(ui->actionExport_Types);

    menu->exec(ui->typesTreeView->mapToGlobal(pt));

    delete menu;
}

void TypesWidget::on_actionExport_Types_triggered()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), Config()->getRecentFolder());
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Can't save file";
        return;
    }
    QTextStream fileOut(&file);
    fileOut << Core()->cmd("tc");
    file.close();

    Config()->setRecentFolder(QFileInfo(fileName).absolutePath());
}

void TypesWidget::on_actionLoad_New_Types_triggered()
{
    LoadNewTypesDialog *dialog = new LoadNewTypesDialog(this);
    dialog->setWindowTitle(tr("Load New Types"));
    dialog->exec();
}
