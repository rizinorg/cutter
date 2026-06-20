#include "CutterDiffWindow.h"

#include "DiffLoadDialog.h"
#include "ui_CutterDiffWindow.h"

#include <Configuration.h>

FunctionListModel::FunctionListModel(QList<FunctionDescription> *list, QObject *parent)
    : list(list), AddressableItemModel<>(parent)
{
}
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

CutterDiffWindow::CutterDiffWindow(QWidget *parent)
    : QMainWindow(parent),
      cutterDiff(new CutterDiff),
      bDiff(new BinDiff(cutterDiff)),
      ui(new Ui::CutterDiffWindow)
{
    ui->setupUi(this);
    ui->splitter->setSizes({ 250, 750 });
    ui->splitter->setStretchFactor(0, 1);
    ui->splitter->setStretchFactor(1, 3);
    cutterDiff->initCores();
    connect(bDiff, &BinDiff::complete, this, &CutterDiffWindow::onBinDiffCompleted);
    connect(ui->actionDiffNewFiles, &QAction::triggered, this,
            &CutterDiffWindow::onActionDiffNewFile);
    ui->tabWidget2->hide();
    setupFonts();
}

CutterDiffWindow::~CutterDiffWindow()
{
    delete ui;
}

void CutterDiffWindow::setupFonts() {}

void CutterDiffWindow::refreshHex(RVA addr)
{
    // if (addr != RVA_INVALID) {
    //     ui->hexDiffTextView->seek(addr);
    // } else {
    //     ui->hexDiffTextView->refresh();
    // }
}

void CutterDiffWindow::onBinDiffCompleted()
{
    if (!bDiff->hasData()) {
        return;
    }

    const QColor perfect = Config()->getColor("gui.match.perfect");
    const QColor partial = Config()->getColor("gui.match.partial");

    listMatch = bDiff->matches();
    listDel = bDiff->mismatch(true);
    listAdd = bDiff->mismatch(false);

    matches = new DiffMatchModel(&listMatch, perfect, partial, this);
    added = new DiffMismatchModel(&listDel, this);
    removed = new DiffMismatchModel(&listAdd, this);

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
    ui->treeViewFcnsB->setModel(modelB);
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
    ui->tabWidget2->show();
}

void CutterDiffWindow::onActionDiffNewFile()
{
    auto loadDiff = new DiffLoadDialog(bDiff, this);
    loadDiff->show();
}

void CutterDiffWindow::addHexDiff()
{
    if (!hexDiff) {
        hexDiff = new HexDiff(cutterDiff, this);
    }
    ui->hexDiffContainer->layout()->addWidget(hexDiff);
    const QFont font = Config()->getFont();
    hexDiff->setMonospaceFont(font);
}
