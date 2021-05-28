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
    viewHeap->setShowGrid(false);
    viewHeap->setSortingEnabled(true);
    viewHeap->setAutoScroll(false);
    viewHeap->verticalHeader()->hide();
    viewHeap->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    ui->verticalLayout_heap->addWidget(viewHeap);
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
    QJsonDocument heapItems = Core()->getHeap();
    beginResetModel();
    values.clear();
    QJsonArray chunks = heapItems["chunks"].toArray();
    for (const QJsonValue &val : chunks) {
        QJsonObject qjo = val.toObject();
        Chunk chunk;
        chunk.offset = qjo["addr"].toVariant().toULongLong();
        chunk.size = qjo["size"].toVariant().toInt();
        chunk.status = qjo["status"].toVariant().toString();
        values.push_back(chunk);
    }
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