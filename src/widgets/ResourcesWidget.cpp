#include "common/Helpers.h"
#include "ResourcesWidget.h"
#include "ui_ListDockWidget.h"
#include "core/MainWindow.h"
#include <QVBoxLayout>

ResourcesModel::ResourcesModel(QList<ResourcesDescription> *resources, QObject *parent)
    : AddressableItemModel<QAbstractListModel>(parent),
      resources(resources)
{
}

int ResourcesModel::rowCount(const QModelIndex &) const
{
    return resources->count();
}

int ResourcesModel::columnCount(const QModelIndex &) const
{
    return Columns::COUNT;
}

QVariant ResourcesModel::data(const QModelIndex &index, int role) const
{
    const ResourcesDescription &res = resources->at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case NAME:
            return res.name;
        case VADDR:
            return RAddressString(res.vaddr);
        case INDEX:
            return QString::number(res.index);
        case TYPE:
            return res.type;
        case SIZE:
            return qhelpers::formatBytecount(res.size);
        case LANG:
            return res.lang;
        default:
            return QVariant();
        }
    case Qt::EditRole:
        switch (index.column()) {
        case NAME:
            return res.name;
        case VADDR:
            return res.vaddr;
        case INDEX:
            return res.index;
        case TYPE:
            return res.type;
        case SIZE:
            return res.size;
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
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

RVA ResourcesModel::address(const QModelIndex &index) const
{
    const ResourcesDescription &res = resources->at(index.row());
    return res.vaddr;
}

ResourcesWidget::ResourcesWidget(MainWindow *main) :
    ListDockWidget(main, ListDockWidget::SearchBarPolicy::HideByDefault)
{
    setObjectName("ResourcesWidget");

    model = new ResourcesModel(&resources, this);
    filterModel = new AddressableFilterProxyModel(model, this);
    filterModel->setSortRole(Qt::EditRole);
    setModels(filterModel);

    ui->treeView->sortByColumn(0, Qt::AscendingOrder);

    showCount(false);

    // Configure widget
    this->setWindowTitle(tr("Resources"));

    connect(Core(), SIGNAL(refreshAll()), this, SLOT(refreshResources()));
}

void ResourcesWidget::refreshResources()
{
    model->beginResetModel();
    resources = Core()->getAllResources();
    model->endResetModel();
}
