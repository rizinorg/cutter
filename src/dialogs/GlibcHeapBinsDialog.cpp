#include "GlibcHeapBinsDialog.h"

#include "Cutter.h"
#include "GlibcHeapInfoDialog.h"
#include "ui_GlibcHeapBinsDialog.h"

#include <HeapBinsGraphView.h>

GlibcHeapBinsDialog::GlibcHeapBinsDialog(RVA mState, MainWindow *main, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::GlibcHeapBinsDialog),
      mState(mState),
      binsModel(new BinsModel(mState, this)),
      graphView(nullptr),
      main(main)
{
    ui->setupUi(this);
    ui->viewBins->setModel(binsModel);
    ui->viewBins->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->viewBins->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->viewBins->verticalHeader()->hide();
    ui->viewBins->resizeColumnsToContents();

    connect(ui->viewBins->selectionModel(), &QItemSelectionModel::currentChanged, this,
            &GlibcHeapBinsDialog::onCurrentChanged);
    connect(ui->lineEdit, &QLineEdit::returnPressed, this,
            &GlibcHeapBinsDialog::showHeapInfoDialog);

    binsModel->reload();
    ui->viewBins->resizeColumnsToContents();

    this->setWindowTitle(tr("Bins info for arena @ ") + rzAddressString(mState));
}

GlibcHeapBinsDialog::~GlibcHeapBinsDialog() {}

void GlibcHeapBinsDialog::onCurrentChanged(const QModelIndex &current, const QModelIndex &prev)
{
    Q_UNUSED(current);
    Q_UNUSED(prev);
    auto currentIndex = ui->viewBins->selectionModel()->currentIndex();
    setChainInfo(currentIndex.row());
    setGraphView(currentIndex.row());
}

void GlibcHeapBinsDialog::setChainInfo(int index)
{
    // get chunks for the selected bin and construct chain info string
    RzListIter *iter;
    RzHeapChunkListItem *item;
    const RzList *chunks = binsModel->getChunks(index);
    QString chainInfo;
    CutterRzListForeach (chunks, iter, RzHeapChunkListItem, item) {
        chainInfo += " → " + rzAddressString(item->addr);
    }

    // Add bin message at the end of the list
    // responsible for messages like corrupted list, double free
    const QString message = binsModel->getBinMessage(index);
    if (!message.isEmpty()) {
        chainInfo += " " + message;
    }

    ui->chainInfoEdit->setPlainText(chainInfo);
}

void GlibcHeapBinsDialog::showHeapInfoDialog()
{
    const QString str = ui->lineEdit->text();
    if (!str.isEmpty()) {
        // summon glibcHeapInfoDialog box with the offset entered
        const RVA offset = Core()->math(str);
        if (!offset) {
            ui->lineEdit->setText(QString());
            return;
        }

        GlibcHeapInfoDialog dialog(offset, QString(), this);
        dialog.exec();
    }
}

void GlibcHeapBinsDialog::setGraphView(int index)
{
    if (graphView) {
        ui->horizontalLayout->removeWidget(graphView);
        delete graphView;
    }
    graphView = new HeapBinsGraphView(this, binsModel->values[index], main);
    ui->horizontalLayout->addWidget(graphView);
    graphView->refreshView();
}

BinsModel::BinsModel(RVA arena_addr, QObject *parent)
    : QAbstractTableModel(parent), arenaAddr(arena_addr)
{
}

void BinsModel::reload()
{
    beginResetModel();
    clearData();
    values = Core()->getHeapBins(arenaAddr);
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
    if (!index.isValid() || index.row() >= values.count()) {
        return QVariant();
    }

    const auto &item = values.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case BinNumColumn:
            return item->bin_num;
        case FdColumn:
            return (item->fd == 0) ? tr("N/A") : rzAddressString(item->fd);
        case BkColumn:
            return (item->bk == 0) ? tr("N/A") : rzAddressString(item->bk);
        case TypeColumn:
            return tr(item->type);
        case CountColumn:
            return rz_list_length(item->chunks);
        case SizeColumn:
            return (item->size == 0) ? tr("N/A") : rzHexString(item->size);
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
            return tr("Fd");
        case BkColumn:
            return tr("Bk");
        case TypeColumn:
            return tr("Type");
        case CountColumn:
            return tr("Chunks count");
        case SizeColumn:
            return tr("Chunks size");
        default:
            return QVariant();
        }

    case Qt::ToolTipRole:
        switch (section) {
        case BinNumColumn:
            return tr("Bin number in RZ_GLIBC_NBINS or fastbinsY array");
        case FdColumn:
            return tr("Pointer to first chunk of the bin");
        case BkColumn:
            return tr("Pointer to last chunk of the bin");
        case TypeColumn:
            return tr("Type of bin");
        case CountColumn:
            return tr("Number of chunks in the bin");
        case SizeColumn:
            return tr("Size of all chunks in the bin");
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
        rz_heap_bin_free(item);
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
