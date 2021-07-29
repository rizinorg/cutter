#include "WindowsHeapWidget.h"
#include "ui_WindowsHeapWidget.h"

WindowsHeapWidget::WindowsHeapWidget(MainWindow *main, QWidget *parent)
    : QWidget(parent), ui(new Ui::WindowsHeapWidget)
{
    ui->setupUi(this);
    viewHeap = ui->tableView;
    viewHeap->setFont(Config()->getFont());
    viewHeap->setModel(modelHeap);
    viewHeap->verticalHeader()->hide();
    // change the scroll mode to ScrollPerPixel
    viewHeap->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    viewHeap->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    connect(Core(), &CutterCore::refreshAll, this, &WindowsHeapWidget::updateContents);
    connect(Core(), &CutterCore::debugTaskStateChanged, this, &WindowsHeapWidget::updateContents);

    refreshDeferrer = dynamic_cast<CutterDockWidget *>(parent)->createRefreshDeferrer(
            [this]() { updateContents(); });

    updateContents();
}

WindowsHeapWidget::~WindowsHeapWidget()
{
    delete ui;
}

void WindowsHeapWidget::updateContents()
{
    if (!refreshDeferrer->attemptRefresh(nullptr) || Core()->isDebugTaskInProgress()) {
        return;
    }

    modelHeap->reload();
    viewHeap->resizeColumnsToContents();
}

WindowsHeapModel::WindowsHeapModel(QObject *parent) : QAbstractTableModel(parent) {}

QVariant WindowsHeapModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= values.count())
        return QVariant();

    const auto &item = values.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case HeaderAddColumn:
            return RAddressString(item.headerAddress);
        case UserAddColumn:
            return RAddressString(item.userAddress);
        case SizeColumn:
            return item.size;
        case UnusedColumn:
            return item.unusedBytes;
        case GranularityColumn:
            return item.granularity;
        case TypeColumn:
            return item.type;
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

QVariant WindowsHeapModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation);
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case HeaderAddColumn:
            return tr("Header Address");
        case UserAddColumn:
            return tr("User Address");
        case SizeColumn:
            return tr("Size");
        case GranularityColumn:
            return tr("Granularity");
        case UnusedColumn:
            return tr("Unused");
        case TypeColumn:
            return tr("Type");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

int WindowsHeapModel::columnCount(const QModelIndex &) const
{
    return ColumnCount;
}

int WindowsHeapModel::rowCount(const QModelIndex &) const
{
    return this->values.size();
}

void WindowsHeapModel::reload()
{
    beginResetModel();
    values.clear();
    values = Core()->getHeapBlocks();
    endResetModel();
}