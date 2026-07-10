#include "CutterDiffWindow.h"

#include "DiffLoadDialog.h"
#include "ui_CutterDiffWindow.h"

#include <QApplication>
#include <QClipboard>

#include <Configuration.h>

FunctionListModel::FunctionListModel(QList<FunctionDescription> *list, QObject *parent)
    : AddressableItemModel<>(parent), list(list)
{
}

FunctionListModel::~FunctionListModel() {}

QModelIndex FunctionListModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return QModelIndex();
    }

    if (row < 0 || row >= list->size()) {
        return QModelIndex();
    }

    if (column < 0 || column >= ColumnCount) {
        return QModelIndex();
    }

    return createIndex(row, column);
}

QModelIndex FunctionListModel::parent(const QModelIndex &) const
{
    return QModelIndex();
}

int FunctionListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return list->size();
}

int FunctionListModel::columnCount(const QModelIndex &) const
{
    return ColumnCount;
}

QVariant FunctionListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (index.row() < 0 || index.row() >= list->size()) {
        return QVariant();
    }

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case Name:
            return list->at(index.row()).name;
        case Offset:
            return list->at(index.row()).offset;
        default:
            return "unknown";
        }
    case Qt::ToolTipRole:
        switch (index.column()) {
        case Name:
            return list->at(index.row()).name;
        case Offset:
            return list->at(index.row()).offset;
        default:
            return "unknown";
        }
    default:
        return QVariant();
    }
}

QVariant FunctionListModel::headerData(int section, Qt::Orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case Name:
            return "Function Name";
        case Offset:
            return "Offset";
        default:
            return QVariant();
        }
    case Qt::ToolTipRole:
        switch (section) {
        case Name:
            return "Name of the function.";
        case Offset:
            return "Address of the function.";
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

RVA FunctionListModel::address(const QModelIndex &index) const
{
    return list->at(index.row()).offset;
}

DiffMatchModel::DiffMatchModel(QList<BinDiffMatchDescription> *list, QColor cPerf, QColor cPart,
                               QObject *parent)
    : QAbstractListModel(parent), list(list), perfect(cPerf), partial(cPart)
{
}

DiffMatchModel::~DiffMatchModel() {}

int DiffMatchModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return list->size();
}

int DiffMatchModel::columnCount(const QModelIndex &) const
{
    return DiffMatchModel::ColumnCount;
}

QPair<RVA, RVA> DiffMatchModel::address(const QModelIndex &index) const
{
    return QPair<RVA, RVA>(list->at(index.row()).original.offset,
                           list->at(index.row()).modified.offset);
}

QVariant DiffMatchModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= list->count()) {
        return QVariant();
    }

    const BinDiffMatchDescription &entry = list->at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case NameOrig:
            return entry.original.name;
        case SizeOrig:
            return QString::asprintf("%llu (%#llx)", entry.original.linearSize,
                                     entry.original.linearSize);
        case AddressOrig:
            return rzAddressString(entry.original.offset);
        case Similarity:
            return QString::asprintf("%.2f (%.2f %%)", entry.similarity, entry.similarity * 100.0);
        case AddressMod:
            return rzAddressString(entry.modified.offset);
        case SizeMod:
            return QString::asprintf("%llu (%#llx)", entry.modified.linearSize,
                                     entry.modified.linearSize);
        case NameMod:
            return entry.modified.name;
        default:
            return QVariant();
        }

    case Qt::ToolTipRole: {
        switch (index.column()) {
        case NameOrig:
            return entry.original.name;
        case SizeOrig:
            return QString::asprintf("%llu (%#llx)", entry.original.linearSize,
                                     entry.original.linearSize);
        case AddressOrig:
            return rzAddressString(entry.original.offset);
        case Similarity:
            return entry.simtype;
        case AddressMod:
            return rzAddressString(entry.modified.offset);
        case SizeMod:
            return QString::asprintf("%llu (%#llx)", entry.modified.linearSize,
                                     entry.modified.linearSize);
        case NameMod:
            return entry.modified.name;
        default:
            return QVariant();
        }
    }

    case Qt::BackgroundRole: {
        return gradientByRatio(entry.similarity);
    }

    default:
        return QVariant();
    }
}

QVariant DiffMatchModel::headerData(int section, Qt::Orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case NameOrig:
            return tr("Name (A)");
        case SizeOrig:
            return tr("Size (A)");
        case AddressOrig:
            return tr("Address (A)");
        case Similarity:
            return tr("Similarity");
        case AddressMod:
            return tr("Address (B)");
        case SizeMod:
            return tr("Size (B)");
        case NameMod:
            return tr("Name (B)");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

QColor DiffMatchModel::gradientByRatio(const double ratio) const
{
    const float red = partial.redF() + (ratio * (perfect.redF() - partial.redF()));
    const float green = partial.greenF() + (ratio * (perfect.greenF() - partial.greenF()));
    const float blue = partial.blueF() + (ratio * (perfect.blueF() - partial.blueF()));
    return QColor::fromRgbF(red, green, blue);
}

DiffMismatchModel::DiffMismatchModel(QList<FunctionDescription> *list, QObject *parent)
    : QAbstractListModel(parent), list(list)
{
}

DiffMismatchModel::~DiffMismatchModel() {}

int DiffMismatchModel::rowCount(const QModelIndex &) const
{
    return list->count();
}

int DiffMismatchModel::columnCount(const QModelIndex &) const
{
    return DiffMismatchModel::ColumnCount;
}

QVariant DiffMismatchModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= list->count()) {
        return QVariant();
    }

    const FunctionDescription &entry = list->at(index.row());

    switch (role) {
    case Qt::ToolTipRole:
        /* fall-thru */
    case Qt::DisplayRole:
        switch (index.column()) {
        case FuncName:
            return entry.name;
        case FuncAddress:
            return rzAddressString(entry.offset);
        case FuncLinearSize:
            return QString::asprintf("%llu (%#llx)", entry.linearSize, entry.linearSize);
        case FuncNargs:
            return QString::asprintf("%llu", entry.nargs);
        case FuncNlocals:
            return QString::asprintf("%llu", entry.nlocals);
        case FuncNbbs:
            return QString::asprintf("%llu", entry.nbbs);
        case FuncCalltype:
            return entry.calltype;
        case FuncEdges:
            return QString::asprintf("%llu", entry.edges);
        case FuncStackframe:
            return QString::asprintf("%llu", entry.stackframe);
        default:
            return QVariant();
        }

    default:
        return QVariant();
    }
}

QVariant DiffMismatchModel::headerData(int section, Qt::Orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case FuncName:
            return tr("Name");
        case FuncAddress:
            return tr("Address");
        case FuncLinearSize:
            return tr("Linear Size");
        case FuncNargs:
            return tr("Num Args");
        case FuncNlocals:
            return tr("Num Locals");
        case FuncNbbs:
            return tr("Basic Blocks");
        case FuncCalltype:
            return tr("Call Type");
        case FuncEdges:
            return tr("Edges");
        case FuncStackframe:
            return tr("Stackframe");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

CutterDiffWindow::CutterDiffWindow(std::unique_ptr<BinDiff> bDiff, QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::CutterDiffWindow),
      bDiff(std::move(bDiff)),
      cutterDiff(bDiff->cutterDiff.get())
{
    ui->setupUi(this);
    ui->splitter->setSizes({ 250, 750 });
    ui->splitterHexView->setSizes({ 750, 250 });
    ui->treeViewMatches->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeViewMatches, &CutterTreeView::customContextMenuRequested, this,
            &CutterDiffWindow::showContextMenuMatches);
    connect(ui->treeViewMatches, &CutterTreeView::clicked, this, &CutterDiffWindow::selectFunction);
    // connect(bDiff, &BinDiff::complete, this, &CutterDiffWindow::onBinDiffCompleted);
    // connect(ui->actionDiffNewFiles, &QAction::triggered, this,
    //         &CutterDiffWindow::onActionDiffNewFile);
    ui->tabParsing->hide();
    setupFonts();
    showMaximized();
    addLineDiff();
    showDiff();
}

CutterDiffWindow::~CutterDiffWindow()
{
    delete ui;
}

// Work incomplete
void CutterDiffWindow::selectFunction(const QModelIndex &index) {}

void CutterDiffWindow::showContextMenuMatches(const QPoint &pos)
{
    const QModelIndex index = ui->treeViewMatches->indexAt(pos);
    auto addr = matches->address(index);

    QMenu menu(this);

    const QAction *seekTo = menu.addAction("Seek to");
    const QAction *goToAndAlign = menu.addAction("Align HexDiff");
    const QAction *diffFunctionLines = menu.addAction("Line Diff");
    const QAction *copyAddress = menu.addAction("Copy Address");

    const QAction *selected = menu.exec(ui->treeViewMatches->viewport()->mapToGlobal(pos));

    if (selected == seekTo) {
        if (index.column() < DiffMatchModel::AddressMod) {
            hexDiff->seek(addr.first);
        } else {
            hexDiff->seek(addr.second, false);
        }
        ui->tabWidget->setCurrentIndex(3);
    } else if (selected == goToAndAlign) {
        if (addr.first > addr.second) {
            hexDiff->transpose(0, -static_cast<int>(addr.first - addr.second), true);
        } else {
            hexDiff->transpose(0, static_cast<int>(addr.second - addr.first), true);
        }
        ui->tabWidget->setCurrentIndex(3);
        hexDiff->seek(addr.first);
    } else if (selected == diffFunctionLines) {
        lineDiff->fetchFunctionDisas(addr.first, addr.second);
        ui->tabWidget->setCurrentIndex(4);
    } else if (selected == copyAddress) {
        if (index.column() < DiffMatchModel::AddressMod) {
            QApplication::clipboard()->setText(QString::number(addr.first));
        } else {
            QApplication::clipboard()->setText(QString::number(addr.second));
        }
    }
}

void CutterDiffWindow::setupFonts() {}

void CutterDiffWindow::showDiff()
{
    if (!bDiff->hasData()) {
        return;
    }

    const QColor perfect = Config()->getColor(
            "gui.match.perfect"); // needs to be added to either cutter or in rizin
    const QColor partial = Config()->getColor("gui.match.partial");

    listMatch = bDiff->matches();
    listDel = bDiff->mismatch(true);
    listAdd = bDiff->mismatch(false);

    matches = new DiffMatchModel(&listMatch, perfect, partial, this);
    added = new DiffMismatchModel(&listAdd, this);
    removed = new DiffMismatchModel(&listDel, this);

    ui->treeViewMatches->setModel(matches);
    ui->treeViewMatches->sortByColumn(DiffMatchModel::Similarity, Qt::AscendingOrder);
    ui->treeViewMatches->setContextMenuPolicy(Qt::CustomContextMenu);

    ui->treeViewRemoved->setModel(removed);
    ui->treeViewRemoved->sortByColumn(DiffMismatchModel::FuncName, Qt::AscendingOrder);
    ui->treeViewRemoved->setContextMenuPolicy(Qt::CustomContextMenu);

    ui->treeViewAdded->setModel(added);
    ui->treeViewAdded->sortByColumn(DiffMismatchModel::FuncName, Qt::AscendingOrder);
    ui->treeViewAdded->setContextMenuPolicy(Qt::CustomContextMenu);

    fcnsA = cutterDiff->getFunctionList(true);
    fcnsB = cutterDiff->getFunctionList(false);
    modelA = new FunctionListModel(&fcnsA, this);
    modelB = new FunctionListModel(&fcnsB, this);

    ui->treeViewFcnsA->setModel(modelA);
    ui->fcnsALabel->setText(cutterDiff->getFileName(true));
    ui->treeViewFcnsB->setModel(modelB);
    ui->fcnsBLabel->setText(cutterDiff->getFileName(false));
    addHexDiff();
    connect(ui->treeViewFcnsA, &CutterTreeView::clicked, this,
            [this](const QModelIndex &index) { hexDiff->seek(modelA->address(index), true); });
    connect(ui->treeViewFcnsB, &CutterTreeView::clicked, this,
            [this](const QModelIndex &index) { hexDiff->seek(modelB->address(index), false); });

    // Transpose

    connect(ui->shiftUpA, &QPushButton::clicked, this, [this]() { hexDiff->transpose(-1, 0); });
    connect(ui->shiftDownA, &QPushButton::clicked, this, [this]() { hexDiff->transpose(1, 0); });
    connect(ui->shiftUpB, &QPushButton::clicked, this, [this]() { hexDiff->transpose(0, -1); });
    connect(ui->shiftDownB, &QPushButton::clicked, this, [this]() { hexDiff->transpose(0, 1); });
    ui->tabParsing->show();

    // Parsing

    // Info

    ui->copyMD5A->setIcon(QIcon(":/img/icons/copy.svg"));
    ui->copySHA1A->setIcon(QIcon(":/img/icons/copy.svg"));
    ui->copySHA256A->setIcon(QIcon(":/img/icons/copy.svg"));
    ui->copyCRC32A->setIcon(QIcon(":/img/icons/copy.svg"));

    ui->copyMD5B->setIcon(QIcon(":/img/icons/copy.svg"));
    ui->copySHA1B->setIcon(QIcon(":/img/icons/copy.svg"));
    ui->copySHA256B->setIcon(QIcon(":/img/icons/copy.svg"));
    ui->copyCRC32B->setIcon(QIcon(":/img/icons/copy.svg"));

    // Setup Placeholders
    const QString placeholder = tr("Select bytes to display information");

    ui->bytesMD5A->setPlaceholderText(placeholder);
    ui->bytesEntropyA->setPlaceholderText(placeholder);
    ui->bytesSHA1A->setPlaceholderText(placeholder);
    ui->bytesSHA256A->setPlaceholderText(placeholder);
    ui->bytesCRC32A->setPlaceholderText(placeholder);

    ui->bytesMD5B->setPlaceholderText(placeholder);
    ui->bytesEntropyB->setPlaceholderText(placeholder);
    ui->bytesSHA1B->setPlaceholderText(placeholder);
    ui->bytesSHA256B->setPlaceholderText(placeholder);
    ui->bytesCRC32B->setPlaceholderText(placeholder);

    // HexDiff signals
    connect(hexDiff, &HexDiff::selectionChanged, this, &CutterDiffWindow::selectionChanged);
}

// void CutterDiffWindow::onActionDiffNewFile()
// {
//     auto loadDiff = new DiffLoadDialog(bDiff, this);
//     loadDiff->show();
// }

void CutterDiffWindow::addHexDiff()
{
    if (!hexDiff) {
        hexDiff = new HexDiff(cutterDiff, this);
    }
    ui->hexDiffContainer->layout()->addWidget(hexDiff);
    const QFont font = Config()->getFont();
    hexDiff->setMonospaceFont(font);
}

void CutterDiffWindow::addLineDiff()
{
    if (!lineDiff) {
        lineDiff = new LineDiffWidget(cutterDiff, this);
    }
    ui->lineDiffContainer->layout()->addWidget(lineDiff);
}

void CutterDiffWindow::onCopyMD5AClicked()
{
    const QString md5A = ui->bytesMD5A->text();
    QApplication::clipboard()->setText(md5A);
}

void CutterDiffWindow::onCopyShA1AClicked()
{
    const QString sha1A = ui->bytesSHA1A->text();
    QApplication::clipboard()->setText(sha1A);
}

void CutterDiffWindow::onCopyShA256AClicked()
{

    const QString sha256A = ui->bytesSHA256A->text();
    QApplication::clipboard()->setText(sha256A);
}

void CutterDiffWindow::onCopyCrC32AClicked()
{
    const QString crc32A = ui->bytesCRC32A->text();
    QApplication::clipboard()->setText(crc32A);
}

void CutterDiffWindow::onCopyMD5BClicked()
{
    const QString md5B = ui->bytesMD5B->text();
    QApplication::clipboard()->setText(md5B);
}

void CutterDiffWindow::onCopyShA1BClicked()
{
    const QString sha1B = ui->bytesSHA1B->text();
    QApplication::clipboard()->setText(sha1B);
}

void CutterDiffWindow::onCopyShA256BClicked()
{

    const QString sha256B = ui->bytesSHA256B->text();
    QApplication::clipboard()->setText(sha256B);
}

void CutterDiffWindow::onCopyCrC32BClicked()
{
    const QString crc32B = ui->bytesCRC32B->text();
    QApplication::clipboard()->setText(crc32B);
}

void CutterDiffWindow::selectionChanged(HexDiff::Selection selection)
{
    if (selection.empty) {
        clearParseWindow();
    } else {
        updateParseWindow(selection);
    }
}

void CutterDiffWindow::updateParseWindow(HexDiff::Selection selection)
{
    const int size = selection.endAddress - selection.startAddress + 1;
    if (ui->tabParsing->currentIndex() == 1) {
        RzHashSize digestSize = 0;
        const CutterDiffLocked cutterDiff(this->cutterDiff);
        const ut64 oldOffsetA = cutterDiff.coreA->offset;
        const ut64 oldOffsetB = cutterDiff.coreB->offset;
        rz_core_seek(cutterDiff.coreA, selection.startAddress, true);
        rz_core_seek(cutterDiff.coreB, selection.startAddressB, true);
        const ut8 *blockA = cutterDiff.coreA->block;
        const ut8 *blockB = cutterDiff.coreB->block;
        char *digest = rz_hash_cfg_calculate_small_block_string(cutterDiff.coreA->hash, "md5",
                                                                blockA, size, &digestSize, false);
        ui->bytesMD5A->setText(QString(digest));
        free(digest);

        digest = rz_hash_cfg_calculate_small_block_string(cutterDiff.coreB->hash, "md5", blockB,
                                                          size, &digestSize, false);
        ui->bytesMD5B->setText(QString(digest));
        free(digest);

        digest = rz_hash_cfg_calculate_small_block_string(cutterDiff.coreA->hash, "sha1", blockA,
                                                          size, &digestSize, false);
        ui->bytesSHA1A->setText(QString(digest));
        free(digest);

        digest = rz_hash_cfg_calculate_small_block_string(cutterDiff.coreB->hash, "sha1", blockB,
                                                          size, &digestSize, false);
        ui->bytesSHA1B->setText(QString(digest));
        free(digest);

        digest = rz_hash_cfg_calculate_small_block_string(cutterDiff.coreA->hash, "sha256", blockA,
                                                          size, &digestSize, false);
        ui->bytesSHA256A->setText(QString(digest));
        free(digest);

        digest = rz_hash_cfg_calculate_small_block_string(cutterDiff.coreB->hash, "sha256", blockB,
                                                          size, &digestSize, false);
        ui->bytesSHA256B->setText(QString(digest));
        free(digest);

        digest = rz_hash_cfg_calculate_small_block_string(cutterDiff.coreA->hash, "crc32", blockA,
                                                          size, &digestSize, false);
        ui->bytesCRC32A->setText(QString(digest));
        free(digest);

        digest = rz_hash_cfg_calculate_small_block_string(cutterDiff.coreB->hash, "crc32", blockB,
                                                          size, &digestSize, false);
        ui->bytesCRC32B->setText(QString(digest));
        free(digest);

        digest = rz_hash_cfg_calculate_small_block_string(cutterDiff.coreA->hash, "entropy", blockA,
                                                          size, &digestSize, false);
        ui->bytesEntropyA->setText(QString(digest));
        free(digest);

        digest = rz_hash_cfg_calculate_small_block_string(cutterDiff.coreB->hash, "entropy", blockB,
                                                          size, &digestSize, false);
        ui->bytesEntropyB->setText(QString(digest));
        free(digest);

        rz_core_seek(cutterDiff.coreA, oldOffsetA, true);
        rz_core_seek(cutterDiff.coreB, oldOffsetB, true);
        ui->bytesMD5A->setCursorPosition(0);
        ui->bytesSHA1A->setCursorPosition(0);
        ui->bytesSHA256A->setCursorPosition(0);
        ui->bytesCRC32A->setCursorPosition(0);
        ui->bytesMD5B->setCursorPosition(0);
        ui->bytesSHA1B->setCursorPosition(0);
        ui->bytesSHA256B->setCursorPosition(0);
        ui->bytesCRC32B->setCursorPosition(0);
    }
}

void CutterDiffWindow::clearParseWindow()
{
    ui->bytesEntropyA->setText("");
    ui->bytesMD5A->setText("");
    ui->bytesSHA1A->setText("");
    ui->bytesSHA256A->setText("");
    ui->bytesCRC32A->setText("");

    ui->bytesEntropyB->setText("");
    ui->bytesMD5B->setText("");
    ui->bytesSHA1B->setText("");
    ui->bytesSHA256B->setText("");
    ui->bytesCRC32B->setText("");
}
