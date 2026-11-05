#include "DiffMisMatchWidget.h"

#include <QVBoxLayout>

#include <shortcuts/ShortcutManager.h>

DiffMismatchProxyModel::DiffMismatchProxyModel(QObject *parent) : QSortFilterProxyModel(parent) {}

bool DiffMismatchProxyModel::filterAcceptsRow(int sourceRow,
                                              const QModelIndex & /*sourceParent*/) const
{
    const auto *model = qobject_cast<const DiffMismatchModel *>(sourceModel());

    if (!model) {
        return false;
    }

    if (qhelpers::filterRegexEmpty(this)) {
        return true;
    }
    for (int column = 0; column < DiffMismatchModel::ColumnCount; ++column) {

        const QModelIndex index = model->index(sourceRow, column);

        const QString text = index.data(Qt::DisplayRole).toString();

        if (qhelpers::filterStringContains(text, this)) {
            return true;
        }
    }

    return false;
}

bool DiffMismatchProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    switch (left.column()) {
    case DiffMismatchModel::FuncName:
        return left.data(Qt::UserRole)
                       .toString()
                       .compare(right.data(Qt::UserRole).toString(), Qt::CaseInsensitive)
                < 0;

    case DiffMismatchModel::FuncAddress:
    case DiffMismatchModel::FuncLinearSize:
    case DiffMismatchModel::FuncNargs:
    case DiffMismatchModel::FuncNlocals:
    case DiffMismatchModel::FuncNbbs:
    case DiffMismatchModel::FuncCalltype:
    case DiffMismatchModel::FuncEdges:
    case DiffMismatchModel::FuncStackframe:
        return left.data(Qt::UserRole).toULongLong() < right.data(Qt::UserRole).toULongLong();

    default:
        return QSortFilterProxyModel::lessThan(left, right);
    }
}

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
    case Qt::DisplayRole: {
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
    }
    case Qt::UserRole: {
        switch (index.column()) {
        case FuncName:
            return entry.name;

        case FuncAddress:
            return QVariant::fromValue(entry.offset);

        case FuncLinearSize:
            return QVariant::fromValue(entry.linearSize);

        case FuncNargs:
            return QVariant::fromValue(entry.nargs);

        case FuncNlocals:
            return QVariant::fromValue(entry.nlocals);

        case FuncNbbs:
            return QVariant::fromValue(entry.nbbs);

        case FuncCalltype:
            return QVariant::fromValue(entry.calltype);

        case FuncEdges:
            return QVariant::fromValue(entry.edges);

        case FuncStackframe:
            return QVariant::fromValue(entry.stackframe);

        default:
            return QVariant();
        }
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
      model(new DiffMismatchModel(&list, this)),
      proxyModel(new DiffMismatchProxyModel(this)),
      mismatchesFilter(new QuickFilterView(this))
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    proxyModel->setSourceModel(model);

    treeView->setModel(proxyModel);
    treeView->setSortingEnabled(true);

    // QuickFilter Setup

    connect(mismatchesFilter, &QuickFilterView::filterTextChanged, proxyModel,
            &QSortFilterProxyModel::setFilterWildcard);

    QShortcut *searchShortcut = Shortcuts()->makeQShortcut("General.showFilter", treeView);
    searchShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    QShortcut *clearShortcut = Shortcuts()->makeQShortcut("General.clearFilter", mismatchesFilter);
    clearShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    connect(searchShortcut, &QShortcut::activated, mismatchesFilter, &QuickFilterView::showFilter);
    connect(clearShortcut, &QShortcut::activated, mismatchesFilter, &QuickFilterView::clearFilter);

    treeView->setIndentation(0);
    treeView->setItemsExpandable(false);
    treeView->setRootIsDecorated(false);
    setLayout(layout);
    layout->addWidget(treeView);
    layout->addWidget(mismatchesFilter);
    connect(cutterDiff, &CutterDiff::diffDataUpdated, this, &DiffMisMatchWidget::reload);
    connect(treeView, &CutterTreeView::doubleClicked, this, [this](const QModelIndex &index) {
        this->diffWindow->showPrevMemoryWidget();
        const QModelIndex sourceIndex = proxyModel->mapToSource(index);
        this->cutterDiff->setCurrentDiffItemIndex(list[sourceIndex.row()].diffItemIndex);
    });
    treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(treeView, &QTreeView::customContextMenuRequested, this, [this](const QPoint &pos) {
        const QModelIndex proxyIndex = treeView->indexAt(pos);

        if (!proxyIndex.isValid()) {
            return;
        }

        const QModelIndex sourceIndex = proxyModel->mapToSource(proxyIndex);

        if (!sourceIndex.isValid()) {
            return;
        }

        diffWindow->showDiffItemContextMenu(treeView->viewport()->mapToGlobal(pos),
                                            list[sourceIndex.row()].diffItemIndex);
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
