#include "TypesWidget.h"
#include "ui_TypesWidget.h"
#include "MainWindow.h"
#include "utils/Helpers.h"

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
            return exp.size;
        case FORMAT:
            return exp.format;
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
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

void TypesModel::beginReloadTypes()
{
    beginResetModel();
}

void TypesModel::endReloadTypes()
{
    endResetModel();
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
    return exp.type.contains(filterRegExp());
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
    default:
        break;
    }

    return left_exp.size < right_exp.size;
}



TypesWidget::TypesWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::TypesWidget)
{
    ui->setupUi(this);

    types_model = new TypesModel(&types, this);
    types_proxy_model = new TypesSortFilterProxyModel(types_model, this);
    ui->typesTreeView->setModel(types_proxy_model);
    ui->typesTreeView->sortByColumn(TypesModel::TYPE, Qt::AscendingOrder);

    setScrollMode();

    connect(Core(), SIGNAL(refreshAll()), this, SLOT(refreshTypes()));
}

TypesWidget::~TypesWidget() {}

void TypesWidget::refreshTypes()
{
    types_model->beginReloadTypes();
    types = Core()->getAllTypes();
    types_model->endReloadTypes();

    qhelpers::adjustColumns(ui->typesTreeView, 3, 0);
}

void TypesWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->typesTreeView);
}

void TypesWidget::on_typesTreeView_doubleClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    // TypeDescription exp = index.data(TypesModel::TypeDescriptionRole).value<TypeDescription>();
    // Core()->seek(exp.vaddr);
}
