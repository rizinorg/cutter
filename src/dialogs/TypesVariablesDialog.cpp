#include "TypesVariablesDialog.h"
#include "ui_TypesVariablesDialog.h"

QString toString(VariableScope scope)
{
    switch (scope) {
    case VariableScope::ALL:
        return TypesVariablesDialog::tr("ALL");
    case VariableScope::GLOBAL:
        return TypesVariablesDialog::tr("GLOBAL");
    case VariableScope::LOCAL:
        return TypesVariablesDialog::tr("LOCAL");
    default:
        return QString();
    }
}

TypesVariablesModel::TypesVariablesModel(QObject *parent) : QAbstractTableModel(parent) {}

int TypesVariablesModel::rowCount(const QModelIndex &) const
{
    return variables.count();
}

int TypesVariablesModel::columnCount(const QModelIndex &) const
{
    return Column::COUNT;
}

QVariant TypesVariablesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= variables.size()) {
        return QVariant();
    }

    const VariableEntry &v = variables.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case Column::NAME:
            return v.name;
        case Column::ADDRESS:
            return v.address;
        case Column::SCOPE:
            return toString(v.scope);
        case Column::FUNCTION:
            return v.function;
        default:
            return QVariant();
        }
    }

    if (role == TypeVariableRole) {
        return QVariant::fromValue(v);
    }
    return QVariant();
}

QVariant TypesVariablesModel::headerData(int section, Qt::Orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        switch (section) {
        case Column::NAME:
            return tr("Name");
        case Column::ADDRESS:
            return tr("Address");
        case Column::SCOPE:
            return tr("Scope");
        case Column::FUNCTION:
            return tr("Function");
        default:
            return QVariant();
        }
    }
    return QVariant();
}

void TypesVariablesModel::updateVariables(const QList<VariableEntry> &newVars)
{
    beginResetModel();
    variables = newVars;
    endResetModel();
}

TypesVariablesProxyModel::TypesVariablesProxyModel(QObject *parent) : QSortFilterProxyModel(parent)
{
}

void TypesVariablesProxyModel::setScope(VariableScope scope)
{
    selectedScope = scope;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    invalidateFilter();
#else
    invalidateRowsFilter();
#endif
}

bool TypesVariablesProxyModel::filterAcceptsRow(int source_row,
                                                const QModelIndex &source_parent) const
{
    if (selectedScope == VariableScope::ALL) {
        return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
    }

    QVariant var = sourceModel()->data(sourceModel()->index(source_row, 0),
                                       TypesVariablesModel::TypeVariableRole);
    if (!var.isValid()) {
        return false;
    }

    VariableEntry v = var.value<VariableEntry>();
    return (v.scope == selectedScope)
            && QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}

bool TypesVariablesProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if (left.column() == TypesVariablesModel::Column::ADDRESS) {
        QVariant varLeft = sourceModel()->data(left, TypesVariablesModel::TypeVariableRole);
        QVariant varRight = sourceModel()->data(right, TypesVariablesModel::TypeVariableRole);

        if (varLeft.isValid() && varRight.isValid()) {
            VariableEntry vLeft = varLeft.value<VariableEntry>();
            VariableEntry vRight = varRight.value<VariableEntry>();

            return vLeft.offset < vRight.offset;
        }
    }

    return QSortFilterProxyModel::lessThan(left, right);
}

TypesVariablesDialog::TypesVariablesDialog(QWidget *parent, const QString &typeName)
    : QDialog(parent), ui(new Ui::TypesVariablesDialog), tree(new CutterTreeWidget(this))
{
    ui->setupUi(this);
    setWindowTitle(tr("Variables: %1").arg(typeName));

    tree->addStatusBar(ui->verticalLayout);

    sourceModel = new TypesVariablesModel(this);
    proxyModel = new TypesVariablesProxyModel(this);
    proxyModel->setSourceModel(sourceModel);

    proxyModel->setFilterKeyColumn(-1);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    ui->treeView->setModel(proxyModel);
    ui->treeView->setSortingEnabled(true);
    ui->treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->treeView->header()->setSectionResizeMode(QHeaderView::Interactive);
    ui->treeView->header()->setStretchLastSection(true);

    ui->quickFilterView->setLabelText(tr("Scope"));
    QComboBox *scopeCombo = ui->quickFilterView->comboBox();
    scopeCombo->addItem(tr("All"), ALL);
    scopeCombo->addItem(tr("Globals Only"), GLOBAL);
    scopeCombo->addItem(tr("Locals Only"), LOCAL);

    auto updateCount = [this]() { tree->showItemsNumber(proxyModel->rowCount()); };

    connect(ui->quickFilterView, &ComboQuickFilterView::filterTextChanged, proxyModel,
            &TypesVariablesProxyModel::setFilterFixedString);
    connect(ui->quickFilterView, &ComboQuickFilterView::filterTextChanged, this, updateCount);

    connect(scopeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this, scopeCombo, updateCount](int index) {
                int scope = scopeCombo->itemData(index).toInt();
                proxyModel->setScope(static_cast<VariableScope>(scope));
                updateCount();
            });

    connect(ui->treeView, &QTreeView::doubleClicked, this,
            &TypesVariablesDialog::onItemDoubleClicked);

    refreshModel(typeName);
    updateCount();
}

void TypesVariablesDialog::refreshModel(const QString &typeName)
{
    QList<VariableEntry> newVariables;

    if (!typeName.isEmpty()) {
        const auto &globals = Core()->getAllGlobals();
        for (const auto &g : globals) {
            if (g.type == typeName) {
                newVariables.append(VariableEntry { g.name, QString("0x%1").arg(g.addr, 0, 16),
                                                    VariableScope::GLOBAL, "", g.addr });
            }
        }

        const auto &fcns = Core()->getAllFunctions();
        for (const auto &f : fcns) {
            const auto &vars = Core()->getVariables(f.offset);
            for (const auto &v : vars) {
                if (v.type == typeName) {
                    newVariables.append(VariableEntry { v.name, v.value, VariableScope::LOCAL,
                                                        f.name, f.offset });
                }
            }
        }
    }

    sourceModel->updateVariables(newVariables);
    qhelpers::adjustColumns(ui->treeView, TypesVariablesModel::Column::COUNT, 0);
}

void TypesVariablesDialog::onItemDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }
    VariableEntry v = index.data(TypesVariablesModel::TypeVariableRole).value<VariableEntry>();
    Core()->seekAndShow(v.offset);
    close();
}

TypesVariablesDialog::~TypesVariablesDialog() {}
