#include "WindowsHeapDialog.h"
#include "ui_WindowsHeapDialog.h"
#include <Cutter.h>
#include <Configuration.h>

WindowsHeapDialog::WindowsHeapDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::WindowsHeapDialog)
{
    ui->setupUi(this);

    viewHeap = ui->tableView;
    viewHeap->setFont(Config()->getFont());
    viewHeap->setModel(modelHeap);
    viewHeap->verticalHeader()->hide();
    // change the scroll mode to ScrollPerPixel
    viewHeap->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    viewHeap->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    updateContents();
}

WindowsHeapDialog::~WindowsHeapDialog()
{
    delete ui;
}

void WindowsHeapDialog::updateContents()
{
    modelHeap->reload();
    viewHeap->resizeColumnsToContents();
}

HeapInfoModel::HeapInfoModel(QObject *parent) : QAbstractTableModel(parent) {}

QVariant HeapInfoModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= values.count())
        return QVariant();

    const auto &item = values.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case BaseColumn:
            return RAddressString(item.base);
        case AllocatedColumn:
            return item.allocated;
        case CommittedColumn:
            return item.committed;
        case BlockCountColumn:
            return item.blockCount;
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

QVariant HeapInfoModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation);
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case BaseColumn:
            return tr("Base Address");
        case AllocatedColumn:
            return tr("Allocated");
        case CommittedColumn:
            return tr("Committed");
        case BlockCountColumn:
            return tr("Block Count");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

int HeapInfoModel::columnCount(const QModelIndex &) const
{
    return ColumnCount;
}

int HeapInfoModel::rowCount(const QModelIndex &) const
{
    return this->values.size();
}

void HeapInfoModel::reload()
{
    beginResetModel();
    values.clear();
    values = Core()->getWindowsHeaps();
    endResetModel();
}