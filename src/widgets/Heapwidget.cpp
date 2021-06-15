#include <QJsonArray>
#include <QJsonObject>
#include "Heapwidget.h"
#include "ui_Heapwidget.h"

#include "core/MainWindow.h"
#include "QHeaderView"

HeapWidget::HeapWidget(MainWindow *main) : CutterDockWidget(main), ui(new Ui::HeapWidget)
{
    ui->setupUi(this);

    viewHeap->setFont(Config()->getFont());
    viewHeap->setModel(modelHeap);
    viewHeap->verticalHeader()->hide();
    viewHeap->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    viewHeap->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    ui->verticalLayout->addWidget(viewHeap);
    connect(Core(), &CutterCore::refreshAll, this, &HeapWidget::updateContents);
    connect(Core(), &CutterCore::debugTaskStateChanged, this, &HeapWidget::updateContents);
}

HeapWidget::~HeapWidget()
{
    delete ui;
}

HeapModel::HeapModel(QObject *parent) : QAbstractTableModel(parent) {}

void HeapWidget::updateContents()
{
    modelHeap->reload();
    viewHeap->resizeColumnsToContents();
}

void HeapModel::reload()
{
    beginResetModel();
    values.clear();
    values = Core()->getHeap();
    endResetModel();
}

int HeapModel::columnCount(const QModelIndex &) const
{
    return ColumnCount;
}

int HeapModel::rowCount(const QModelIndex &) const
{
    return this->values.size();
}

QVariant HeapModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= values.count())
        return QVariant();

    const auto &item = values.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case OffsetColumn:
            return RAddressString(item.offset);
        case SizeColumn:
            return RHexString(item.size);
        case StatusColumn:
            return item.status;
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

QVariant HeapModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation);
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case OffsetColumn:
            return tr("Offset");
        case SizeColumn:
            return tr("Size");
        case StatusColumn:
            return tr("Status");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}