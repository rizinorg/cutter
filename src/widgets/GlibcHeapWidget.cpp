#include "GlibcHeapWidget.h"
#include "ui_GlibcHeapWidget.h"
#include "core/MainWindow.h"
#include "QHeaderView"
#include "dialogs/GlibcHeapInfoDialog.h"

GlibcHeapWidget::GlibcHeapWidget(MainWindow *main, QWidget *parent)
    : QWidget(parent),
      ui(new Ui::GlibcHeapWidget),
      addressableItemContextMenu(this, main),
      main(main)
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
    chunkInfoAction = new QAction(tr("Detailed Chunk Info"), this);
    viewHeap->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(Core(), &CutterCore::refreshAll, this, &GlibcHeapWidget::updateContents);
    connect(Core(), &CutterCore::debugTaskStateChanged, this, &GlibcHeapWidget::updateContents);
    connect(viewHeap, &QAbstractItemView::doubleClicked, this, &GlibcHeapWidget::onDoubleClicked);
    connect<void (QComboBox::*)(int)>(arenaSelectorView, &QComboBox::currentIndexChanged, this,
                                      &GlibcHeapWidget::onArenaSelected);
    connect(viewHeap, &QWidget::customContextMenuRequested, this,
            &GlibcHeapWidget::customMenuRequested);
    connect(viewHeap->selectionModel(), &QItemSelectionModel::currentChanged, this,
            &GlibcHeapWidget::onCurrentChanged);
    connect(chunkInfoAction, &QAction::triggered, this, &GlibcHeapWidget::viewChunkInfo);
    addressableItemContextMenu.addAction(chunkInfoAction);
    addActions(addressableItemContextMenu.actions());
    refreshDeferrer = dynamic_cast<CutterDockWidget *>(parent)->createRefreshDeferrer(
            [this]() { updateContents(); });
}

GlibcHeapWidget::~GlibcHeapWidget()
{
    delete ui;
}

HeapModel::HeapModel(QObject *parent) : QAbstractTableModel(parent) {}

void GlibcHeapWidget::updateArenas()
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
void GlibcHeapWidget::onArenaSelected(int index)
{
    if (index == -1) {
        modelHeap->arena_addr = 0;
    } else {
        modelHeap->arena_addr = arenas[index].offset;
    }
    updateChunks();
}
void GlibcHeapWidget::updateContents()
{
    if (!refreshDeferrer->attemptRefresh(nullptr) || Core()->isDebugTaskInProgress()) {
        return;
    }
    updateArenas();
    updateChunks();
}

void GlibcHeapWidget::updateChunks()
{
    modelHeap->reload();
    viewHeap->resizeColumnsToContents();
}

void GlibcHeapWidget::customMenuRequested(QPoint pos)
{
    addressableItemContextMenu.exec(viewHeap->viewport()->mapToGlobal(pos));
}

void HeapModel::reload()
{
    beginResetModel();
    values.clear();
    values = Core()->getHeapChunks(arena_addr);
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

void GlibcHeapWidget::onDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }
    int column = index.column();
    if (column == HeapModel::OffsetColumn) {
        QString item = index.data().toString();
        Core()->seek(item);
        main->showMemoryWidget(MemoryWidgetType::Hexdump);
    }
}

void GlibcHeapWidget::onCurrentChanged(const QModelIndex &current, const QModelIndex &prev)
{
    Q_UNUSED(current)
    Q_UNUSED(prev)
    auto currentIndex = viewHeap->selectionModel()->currentIndex();
    QString offsetString =
            currentIndex.sibling(currentIndex.row(), HeapModel::OffsetColumn).data().toString();
    addressableItemContextMenu.setTarget(Core()->math(offsetString));
}

void GlibcHeapWidget::viewChunkInfo()
{
    auto currentIndex = viewHeap->selectionModel()->currentIndex();
    QString offsetString =
            currentIndex.sibling(currentIndex.row(), HeapModel::OffsetColumn).data().toString();
    QString status =
            currentIndex.sibling(currentIndex.row(), HeapModel::StatusColumn).data().toString();
    GlibcHeapInfoDialog heapInfoDialog(Core()->math(offsetString), status, this);
    heapInfoDialog.exec();
}
