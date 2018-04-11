#include <QTreeWidget>
#include "RelocsWidget.h"
#include "ui_RelocsWidget.h"
#include "MainWindow.h"
#include "utils/Helpers.h"

RelocsModel::RelocsModel(QList<RelocDescription> *relocs, QObject *parent) :
    QAbstractTableModel(parent),
    relocs(relocs)
{}

int RelocsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid()? 0 : relocs->count();
}

int RelocsModel::columnCount(const QModelIndex&) const
{
    return COUNT;
}

QVariant RelocsModel::data(const QModelIndex &index, int role) const
{
    const RelocDescription &reloc = relocs->at(index.row());
    switch (role)
    {
    case AddressRole:
        return reloc.vaddr;
    case Qt::DisplayRole:
        switch (index.column())
        {
        case VADDR:
            return RAddressString(reloc.vaddr);
        case TYPE:
            return reloc.type;
        case NAME:
            return reloc.name;
        default:
            break;
        }
    default:
        break;
    }
    return QVariant();
}

QVariant RelocsModel::headerData(int section, Qt::Orientation, int role) const
{
    if(role == Qt::DisplayRole)
        switch (section)
        {
        case VADDR:
            return tr("Address");
        case TYPE:
            return tr("Type");
        case NAME:
            return tr("Name");
        }
    return QVariant();
}

void RelocsModel::beginReload()
{
    beginResetModel();
}

void RelocsModel::endReload()
{
    endResetModel();
}


RelocsWidget::RelocsWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::RelocsWidget),
    model(new RelocsModel(&relocs, this))
{
    ui->setupUi(this);

    ui->relocsTableView->setModel(model);

    setScrollMode();

    connect(Core(), SIGNAL(refreshAll()), this, SLOT(refreshRelocs()));
}

RelocsWidget::~RelocsWidget() {}

void RelocsWidget::on_relocsTableView_doubleClicked(const QModelIndex &index)
{
    Core()->seek(index.data(RelocsModel::AddressRole).toLongLong());
}

void RelocsWidget::refreshRelocs()
{
    model->beginReload();
    relocs = Core()->getAllRelocs();
    model->endReload();
    ui->relocsTableView->resizeColumnsToContents();
}

void RelocsWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->relocsTableView);
}
