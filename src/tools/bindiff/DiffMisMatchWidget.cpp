#include "DiffMisMatchWidget.h"

#include <QVBoxLayout>

DiffMismatchModel::DiffMismatchModel(QList<FunctionDescription> *list, QObject *parent)
    : AddressableItemModel(parent), list(list)
{
}

DiffMismatchModel::~DiffMismatchModel() {}

QModelIndex DiffMismatchModel::index(int row, int column, const QModelIndex &parent) const
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

QModelIndex DiffMismatchModel::parent(const QModelIndex &) const
{
    return QModelIndex();
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

RVA DiffMismatchModel::address(const QModelIndex &index) const
{
    return list->at(index.row()).offset;
}

void DiffMismatchModel::reload()
{
    beginResetModel();
    endResetModel();
}

DiffMisMatchWidget::DiffMisMatchWidget(CutterDiff *cutterDiff, CutterDiffWindow *parent,
                                       bool original)
    : CutterDiffWidget(cutterDiff, parent),
      original(original),
      treeView(new CutterTreeView(this)),
      model(new DiffMismatchModel(&list, this))
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    treeView->setIndentation(0);
    treeView->setItemsExpandable(false);
    treeView->setRootIsDecorated(false);
    treeView->setModel(model);
    setLayout(layout);
    layout->addWidget(treeView);
    connect(cutterDiff, &CutterDiff::diffDataUpdated, this, &DiffMisMatchWidget::reload);
    connect(treeView, &CutterTreeView::doubleClicked, this, [this](const QModelIndex &index) {
        this->cutterDiff->setCurrentDiffItemIndex(list[index.row()].diffItemIndex);
    });
}

void DiffMisMatchWidget::reload()
{
    list.clear();
    for (int i = 0; i < cutterDiff->getDiffItemList().size(); i++) {
        const CutterDiffItem &diffItem = cutterDiff->getDiffItemList()[i];
        if (diffItem.getType() == (original ? DiffItemRemoved : DiffItemAdded)) {
            FunctionDescription desc = original ? diffItem.functionA() : diffItem.functionB();
            desc.diffItemIndex = i;
            list.push_back(desc);
        }
    }
    model->reload();
}
