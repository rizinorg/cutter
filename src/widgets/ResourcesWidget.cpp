#include "ResourcesWidget.h"

#include "common/Helpers.h"
#include "core/MainWindow.h"
#include "ui_ListDockWidget.h"

#include <QVBoxLayout>

ResourcesModel::ResourcesModel(QObject *parent) : AddressableItemModel<QAbstractListModel>(parent)
{
}

int ResourcesModel::rowCount(const QModelIndex &) const
{
    return resources.count();
}

int ResourcesModel::columnCount(const QModelIndex &) const
{
    return Columns::COUNT;
}

QVariant ResourcesModel::data(const QModelIndex &index, int role) const
{
    const ResourcesDescription &res = resources.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case NAME:
            return res.name;
        case VADDR:
            return rzAddressString(res.vaddr);
        case INDEX:
            return QString::number(res.index);
        case TYPE:
            return res.type;
        case SIZE:
            return qhelpers::formatByteCount(res.size);
        case LANG:
            return res.lang;
        case COMMENT:
            return Core()->getCommentAt(res.vaddr);
        default:
            return QVariant();
        }
    case Qt::EditRole:
        switch (index.column()) {
        case NAME:
            return res.name;
        case VADDR:
            return static_cast<quint64>(res.vaddr);
        case INDEX:
            return static_cast<quint64>(res.index);
        case TYPE:
            return res.type;
        case SIZE:
            return static_cast<quint64>(res.size);
        case LANG:
            return res.lang;
        default:
            return QVariant();
        }
    case Qt::UserRole:
        return QVariant::fromValue(res);
    default:
        return QVariant();
    }
}

QVariant ResourcesModel::headerData(int section, Qt::Orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case NAME:
            return tr("Name");
        case VADDR:
            return tr("Vaddr");
        case INDEX:
            return tr("Index");
        case TYPE:
            return tr("Type");
        case SIZE:
            return tr("Size");
        case LANG:
            return tr("Lang");
        case COMMENT:
            return tr("Comment");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

RVA ResourcesModel::address(const QModelIndex &index) const
{
    const ResourcesDescription &res = resources.at(index.row());
    return res.vaddr;
}

ResourcesWidget::ResourcesWidget(MainWindow *main)
    : ListDockWidget(main),
      model(new ResourcesModel(this)),
      filterModel(new AddressableFilterProxyModel(model, this))
{
    setObjectName("ResourcesWidget");

    filterModel->setSortRole(Qt::EditRole);
    setModels(filterModel);

    ui->treeView->sortByColumn(0, Qt::AscendingOrder);

    // Configure widget
    this->setWindowTitle(tr("Resources"));

    connect(Core(), &CutterCore::refreshAll, this, &ResourcesWidget::refreshResources);
    connect(Core(), &CutterCore::commentsChanged, this,
            [this]() { qhelpers::emitColumnChanged(model, ResourcesModel::COMMENT); });
    connect(ui->quickFilterView, &QuickFilterView::filterTextChanged, this,
            [this] { ui->quickFilterView->setItemCount(filterModel->rowCount()); });
}

void ResourcesWidget::refreshResources()
{
    model->beginResetModel();
    model->resources = Core()->getAllResources();
    model->endResetModel();

    // set the initial item count
    ui->quickFilterView->setItemCount(filterModel->rowCount());
}
