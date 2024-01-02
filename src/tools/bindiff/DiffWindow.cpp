#include "DiffWindow.h"
#include "ui_DiffWindow.h"

#include <core/Cutter.h>
#include "common/Configuration.h"
#include "CutterConfig.h"
#include <rz_th.h>

#include <QFileDialog>
#include <QMessageBox>

DiffMatchModel::DiffMatchModel(QList<BinDiffMatchDescription> *list, QColor cPerf, QColor cPart,
                               QObject *parent)
    : QAbstractListModel(parent), list(list), perfect(cPerf), partial(cPart)
{
}

int DiffMatchModel::rowCount(const QModelIndex &) const
{
    return list->count();
}

int DiffMatchModel::columnCount(const QModelIndex &) const
{
    return DiffMatchModel::ColumnCount;
}

QVariant DiffMatchModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= list->count())
        return QVariant();

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
            return RzAddressString(entry.original.offset);
        case Similarity:
            return QString::asprintf("%.2f (%.2f %%)", entry.similarity, entry.similarity * 100.0);
        case AddressMod:
            return RzAddressString(entry.modified.offset);
        case SizeMod:
            return QString::asprintf("%llu (%#llx)", entry.modified.linearSize,
                                     entry.modified.linearSize);
        case NameMod:
            return entry.modified.name;
        default:
            return QVariant();
        }

    case Qt::ToolTipRole: {
        return entry.simtype;
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
    float red = partial.redF() + (ratio * (perfect.redF() - partial.redF()));
    float green = partial.greenF() + (ratio * (perfect.greenF() - partial.greenF()));
    float blue = partial.blueF() + (ratio * (perfect.blueF() - partial.blueF()));
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
    if (index.row() >= list->count())
        return QVariant();

    const FunctionDescription &entry = list->at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case FuncName:
            return entry.name;
        case FuncAddress:
            return RzAddressString(entry.offset);
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

    case Qt::ToolTipRole: {
        return entry.name;
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

DiffWindow::DiffWindow(BinDiff *bd, QWidget *parent)
    : QDialog(parent), ui(new Ui::DiffWindow), bDiff(bd)
{
    ui->setupUi(this);

    ui->comboBoxShowInfo->addItem(tr("Summary"));
    ui->comboBoxShowInfo->addItem(tr("AAAA"));
    ui->comboBoxShowInfo->addItem(tr("BBBB"));

    listMatch = bDiff->matches();
    listDel = bDiff->mismatch(true);
    listAdd = bDiff->mismatch(false);

    QColor perfect = Config()->getColor("gui.match.perfect");
    QColor partial = Config()->getColor("gui.match.partial");

    modelMatch = new DiffMatchModel(&listMatch, perfect, partial, this);
    modelDel = new DiffMismatchModel(&listDel, this);
    modelAdd = new DiffMismatchModel(&listAdd, this);

    // Matches Table
    ui->tableViewMatch->setModel(modelMatch);
    ui->tableViewMatch->sortByColumn(DiffMatchModel::Similarity, Qt::AscendingOrder);
    ui->tableViewMatch->verticalHeader()->hide();
    ui->tableViewMatch->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableViewMatch->setContextMenuPolicy(Qt::CustomContextMenu);

    // Deletion Table
    ui->tableViewRem->setModel(modelDel);
    ui->tableViewRem->sortByColumn(DiffMismatchModel::FuncName, Qt::AscendingOrder);
    ui->tableViewRem->verticalHeader()->hide();
    ui->tableViewRem->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableViewRem->setContextMenuPolicy(Qt::CustomContextMenu);

    // Addition Table
    ui->tableViewAdd->setModel(modelAdd);
    ui->tableViewAdd->sortByColumn(DiffMismatchModel::FuncName, Qt::AscendingOrder);
    ui->tableViewAdd->verticalHeader()->hide();
    ui->tableViewAdd->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableViewAdd->setContextMenuPolicy(Qt::CustomContextMenu);
}

DiffWindow::~DiffWindow() {}

void DiffWindow::actionExport_as_JSON() {}

void DiffWindow::actionExport_as_Markdown() {}
