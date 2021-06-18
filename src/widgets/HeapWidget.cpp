#include <QJsonArray>
#include <QJsonObject>
#include "HeapWidget.h"
#include "ui_Heapwidget.h"

#include "core/MainWindow.h"
#include "QHeaderView"

HeapWidget::HeapWidget(MainWindow *main) : CutterDockWidget(main), ui(new Ui::HeapWidget)
{
    ui->setupUi(this);

    viewHeap->setFont(Config()->getFont());
    viewHeap->setModel(modelHeap);
    viewHeap->verticalHeader()->hide();
    // change the scroll mode to ScrollPerPixel
    viewHeap->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    viewHeap->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->verticalLayout->addWidget(viewHeap);
    ui->verticalLayout->addWidget(arenaSelectorView);

    connect(Core(), &CutterCore::refreshAll, this, &HeapWidget::updateContents);
    connect(Core(), &CutterCore::debugTaskStateChanged, this, &HeapWidget::updateContents);
    connect(viewHeap, &QAbstractItemView::doubleClicked, this, &HeapWidget::onDoubleClicked);
    connect<void (QComboBox::*)(int)>(arenaSelectorView, &QComboBox::currentIndexChanged, this,
                                      &HeapWidget::onArenaSelected);
}

HeapWidget::~HeapWidget()
{
    delete ui;
}

HeapModel::HeapModel(QObject *parent) : QAbstractTableModel(parent) {}

void HeapWidget::updateArenas()
{
    arenas = Core()->getArenas();
    int currentIndex = arenaSelectorView->currentIndex();
    arenaSelectorView->clear();
    for (auto &arena : arenas) {
        arenaSelectorView->addItem(RAddressString(arena.offset)
                                   + QString(" (" + arena.type + " Arena)"));
    }
    if (arenaSelectorView->count() < currentIndex || currentIndex == -1) {
        currentIndex = 0;
    }
    arenaSelectorView->setCurrentIndex(currentIndex);
}
void HeapWidget::onArenaSelected(int index)
{
    if (index == -1) {
        modelHeap->arena_addr = 0;
    } else {
        modelHeap->arena_addr = arenas[index].offset;
    }
    updateChunks();
}
void HeapWidget::updateContents()
{
    updateArenas();
    updateChunks();
}

void HeapWidget::updateChunks()
{
    modelHeap->reload();
    viewHeap->resizeColumnsToContents();
}

void HeapModel::reload()
{
    beginResetModel();
    values.clear();
    values = Core()->getHeap(arena_addr);
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

void HeapWidget::onDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }
    int column = index.column();
    if (column == HeapModel::OffsetColumn) {
        QString item = index.data().toString();
        Core()->seek(item);
        mainWindow->showMemoryWidget(MemoryWidgetType::Hexdump);
    }
}