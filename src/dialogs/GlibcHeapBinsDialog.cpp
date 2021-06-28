#include "GlibcHeapBinsDialog.h"
#include "ui_GlibcHeapBinsDialog.h"

GlibcHeapBinsDialog::GlibcHeapBinsDialog(RVA m_state, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::GlibcHeapBinsDialog),
      m_state(m_state),
      binsModel(new BinsModel(m_state, this))
{
    ui->setupUi(this);
    ui->viewBins->setModel(binsModel);
    ui->viewBins->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->viewBins->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->viewBins->verticalHeader()->hide();
    binsModel->reload();
    ui->viewBins->resizeColumnsToContents();
    this->setWindowTitle(tr("Bins info for arena @ ") + RAddressString(m_state));
    connect(ui->viewBins->selectionModel(), &QItemSelectionModel::currentChanged, this,
            &GlibcHeapBinsDialog::onCurrentChanged);
}

GlibcHeapBinsDialog::~GlibcHeapBinsDialog()
{
    delete ui;
}

void GlibcHeapBinsDialog::onCurrentChanged(const QModelIndex &current, const QModelIndex &prev)
{
    Q_UNUSED(current);
    Q_UNUSED(prev);
    auto currentIndex = ui->viewBins->selectionModel()->currentIndex();
    setChainInfo(currentIndex.row());
}
void GlibcHeapBinsDialog::setChainInfo(int index)
{
    RzListIter *iter;
    RzHeapChunkListItem *item;
    RzList *chunks = binsModel->getChunks(index);
    QString chainInfo;
    CutterRListForeach(chunks, iter, RzHeapChunkListItem, item)
    {
        chainInfo += " → " + RAddressString(item->addr);
    }
    QString message = binsModel->getBinMessage(index);
    if (!message.isEmpty()) {
        chainInfo += " " + message;
    }
    ui->chainInfoEdit->setPlainText(chainInfo);
}
BinsModel::BinsModel(RVA arena_addr, QObject *parent)
    : QAbstractTableModel(parent), arena_addr(arena_addr)
{
}
void BinsModel::reload()
{
    beginResetModel();
    clearData();
    values = Core()->getHeapBins(arena_addr);
    endResetModel();
}
int BinsModel::rowCount(const QModelIndex &) const
{
    return values.size();
}
int BinsModel::columnCount(const QModelIndex &) const
{
    return ColumnCount;
}
QVariant BinsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= values.count())
        return QVariant();
    const auto &item = values.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case BinNumColumn:
            return item->bin_num;
        case FdColumn:
            return RAddressString(item->fd);
        case BkColumn:
            return RAddressString(item->bk);
        case TypeColumn:
            return QString(item->type);
        case CountColumn:
            return rz_list_length(item->chunks);
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}
QVariant BinsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation);
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case BinNumColumn:
            return tr("#");
        case FdColumn:
            return tr("fd");
        case BkColumn:
            return tr("bk");
        case TypeColumn:
            return tr("type");
        case CountColumn:
            return tr("#chunks");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

void BinsModel::clearData()
{
    for (auto item : values) {
        rz_heap_bin_free_64(item);
    }
}
RzList *BinsModel::getChunks(int index)
{
    return values[index]->chunks;
}
QString BinsModel::getBinMessage(int index)
{
    if (values[index]->message) {
        return QString(values[index]->message);
    } else {
        return QString();
    }
}
