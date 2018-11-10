#include "common/Helpers.h"
#include "ResourcesWidget.h"
#include "MainWindow.h"
#include <QVBoxLayout>

ResourcesModel::ResourcesModel(QList<ResourcesDescription> *resources, QObject *parent)
    : QAbstractListModel(parent),
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
            return res.index;
        case TYPE:
            return res.type;
        case SIZE:
            return qhelpers::formatBytecount(res.size);
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

ResourcesWidget::ResourcesWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action)
{
    setObjectName("ResourcesWidget");

    model = new ResourcesModel(&resources, this);

    // Configure widget
    this->setWindowTitle(tr("Resources"));

    // Add resources tree view
    view = new CutterTreeView(this);
    view->setModel(model);
    view->show();
    this->setWidget(view);

    connect(Core(), SIGNAL(refreshAll()), this, SLOT(refreshResources()));
    connect(view, SIGNAL(doubleClicked(const QModelIndex &)), this,
            SLOT(onDoubleClicked(const QModelIndex &)));
}

void ResourcesWidget::refreshResources()
{
    model->beginResetModel();
    resources = Core()->getAllResources();
    model->endResetModel();
}

void ResourcesWidget::onDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    ResourcesDescription res = index.data(Qt::UserRole).value<ResourcesDescription>();
    Core()->seek(res.vaddr);
}
