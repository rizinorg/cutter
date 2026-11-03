#include "FunctionDiffNavWidget.h"

#include <QSplitter>
#include <QVBoxLayout>

#include <shortcuts/ShortcutManager.h>

FunctionListProxyModel::FunctionListProxyModel(QObject *parent) : QSortFilterProxyModel(parent) {}

bool FunctionListProxyModel::filterAcceptsRow(int sourceRow,
                                              const QModelIndex & /*sourceParent*/) const
{
    const auto *model = qobject_cast<const FunctionListModel *>(sourceModel());

    if (!model) {
        return false;
    }

    if (qhelpers::filterRegexEmpty(this)) {
        return true;
    }
    for (int column = 0; column < FunctionListModel::ColumnCount; ++column) {

        const QModelIndex index = model->index(sourceRow, column);

        const QString text = index.data(Qt::DisplayRole).toString();

        if (qhelpers::filterStringContains(text, this)) {
            return true;
        }
    }

    return false;
}

bool FunctionListProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    switch (left.column()) {
    case FunctionListModel::Name:
        return left.data(Qt::UserRole)
                       .toString()
                       .compare(right.data(Qt::UserRole).toString(), Qt::CaseInsensitive)
                < 0;

    case FunctionListModel::Offset:
        return left.data(Qt::UserRole).toULongLong() < right.data(Qt::UserRole).toULongLong();

    default:
        return QSortFilterProxyModel::lessThan(left, right);
    }
}

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
            return rzAddressString(list->at(index.row()).offset);
        default:
            return "unknown";
        }
    case Qt::UserRole:
        switch (index.column()) {
        case Name:
            return list->at(index.row()).name;
        case Offset:
            return QVariant::fromValue(list->at(index.row()).offset);
        default:
            return QVariant();
        }
    case Qt::ToolTipRole:
        switch (index.column()) {
        case Name:
            return list->at(index.row()).name;
        case Offset:
            return rzAddressString(list->at(index.row()).offset);
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
            return "Name";
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

void FunctionListModel::reload()
{
    beginResetModel();
    endResetModel();
}

FunctionDiffNavWidget::FunctionDiffNavWidget(CutterDiff *cutterDiff, CutterDiffWindow *parent)
    : CutterDiffWidget(cutterDiff, parent),
      labelFileA(new QLabel(this)),
      labelFileB(new QLabel(this)),
      treeViewA(new CutterTreeView(this)),
      treeViewB(new CutterTreeView(this)),
      modelA(new FunctionListModel(&fcnsA, this)),
      modelB(new FunctionListModel(&fcnsB, this)),
      proxyModelA(new FunctionListProxyModel(this)),
      proxyModelB(new FunctionListProxyModel(this)),
      filterA(new QuickFilterView(this)),
      filterB(new QuickFilterView(this))
{
    auto vBox = new QVBoxLayout(this);
    vBox->setContentsMargins(0, 0, 0, 0);
    vBox->setSpacing(0);

    auto splitter = new QSplitter(Qt::Vertical, this);
    splitter->setContentsMargins(0, 0, 0, 0);
    vBox->addWidget(splitter);

    // Function container A
    auto containerA = new QWidget(this);
    auto vBoxA = new QVBoxLayout(containerA);
    vBoxA->setContentsMargins(0, 0, 0, 0);
    vBoxA->setSpacing(0);

    vBoxA->addWidget(labelFileA);
    vBoxA->addWidget(treeViewA);
    vBoxA->addWidget(filterA);

    splitter->addWidget(containerA);

    // Function container B
    auto containerB = new QWidget(this);
    auto vBoxB = new QVBoxLayout(containerB);
    vBoxB->setContentsMargins(0, 0, 0, 0);
    vBoxB->setSpacing(0);

    vBoxB->addWidget(labelFileB);
    vBoxB->addWidget(treeViewB);
    vBoxB->addWidget(filterB);

    splitter->addWidget(containerB);

    proxyModelA->setSourceModel(modelA);

    treeViewA->setModel(proxyModelA);
    treeViewA->setSortingEnabled(true);

    // QuickFilter Setup

    connect(filterA, &QuickFilterView::filterTextChanged, proxyModelA,
            &QSortFilterProxyModel::setFilterWildcard);

    QShortcut *searchShortcutA = Shortcuts()->makeQShortcut("General.showFilter", treeViewA);
    searchShortcutA->setContext(Qt::WidgetWithChildrenShortcut);
    QShortcut *clearShortcutA = Shortcuts()->makeQShortcut("General.clearFilter", filterA);
    clearShortcutA->setContext(Qt::WidgetWithChildrenShortcut);

    connect(searchShortcutA, &QShortcut::activated, filterA, &QuickFilterView::showFilter);
    connect(clearShortcutA, &QShortcut::activated, filterA, &QuickFilterView::clearFilter);

    proxyModelB->setSourceModel(modelB);

    treeViewB->setModel(proxyModelB);
    treeViewB->setSortingEnabled(true);

    // QuickFilter Setup

    connect(filterB, &QuickFilterView::filterTextChanged, proxyModelB,
            &QSortFilterProxyModel::setFilterWildcard);

    QShortcut *searchShortcutB = Shortcuts()->makeQShortcut("General.showFilter", treeViewB);
    searchShortcutB->setContext(Qt::WidgetWithChildrenShortcut);
    QShortcut *clearShortcutB = Shortcuts()->makeQShortcut("General.clearFilter", filterB);
    clearShortcutB->setContext(Qt::WidgetWithChildrenShortcut);

    connect(searchShortcutB, &QShortcut::activated, filterB, &QuickFilterView::showFilter);
    connect(clearShortcutB, &QShortcut::activated, filterB, &QuickFilterView::clearFilter);

    treeViewA->setIndentation(0);
    treeViewB->setIndentation(0);
    treeViewA->setRootIsDecorated(false);
    treeViewB->setRootIsDecorated(false);

    connect(cutterDiff, &CutterDiff::diffDataUpdated, this, &FunctionDiffNavWidget::reload);
    connect(treeViewA, &CutterTreeView::doubleClicked, this, [this](const QModelIndex &index) {
        const QModelIndex sourceIndex = proxyModelA->mapToSource(index);
        this->diffWindow->showPrevMemoryWidget();
        this->cutterDiff->setCurrentDiffItemIndex(fcnsA[sourceIndex.row()].diffItemIndex);
    });
    connect(treeViewB, &CutterTreeView::doubleClicked, this, [this](const QModelIndex &index) {
        const QModelIndex sourceIndex = proxyModelB->mapToSource(index);
        this->diffWindow->showPrevMemoryWidget();
        this->cutterDiff->setCurrentDiffItemIndex(fcnsB[sourceIndex.row()].diffItemIndex);
    });

    treeViewA->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(treeViewA, &QTreeView::customContextMenuRequested, this, [this](const QPoint &pos) {
        const QModelIndex proxyIndex = treeViewA->indexAt(pos);

        if (!proxyIndex.isValid()) {
            return;
        }

        const QModelIndex sourceIndex = proxyModelA->mapToSource(proxyIndex);

        if (!sourceIndex.isValid()) {
            return;
        }

        diffWindow->showDiffItemContextMenu(treeViewA->viewport()->mapToGlobal(pos),
                                            fcnsA[sourceIndex.row()].diffItemIndex);
    });
    treeViewB->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(treeViewB, &QTreeView::customContextMenuRequested, this, [this](const QPoint &pos) {
        const QModelIndex proxyIndex = treeViewB->indexAt(pos);

        if (!proxyIndex.isValid()) {
            return;
        }

        const QModelIndex sourceIndex = proxyModelB->mapToSource(proxyIndex);

        if (!sourceIndex.isValid()) {
            return;
        }

        diffWindow->showDiffItemContextMenu(treeViewB->viewport()->mapToGlobal(pos),
                                            fcnsB[sourceIndex.row()].diffItemIndex);
    });

    reload();
}

void FunctionDiffNavWidget::reload()
{
    fcnsA.clear();
    fcnsB.clear();
    labelFileA->setText(cutterDiff->getFileName(true));
    labelFileB->setText(cutterDiff->getFileName(false));
    const std::vector<CutterDiffItem> &diffItemList = cutterDiff->getDiffItemList();
    for (int i = 0; i < diffItemList.size(); i++) {
        const CutterDiffItem &diffItem = diffItemList[i];
        const DiffItemType type = diffItem.getType();
        switch (type) {
        case DiffItemRemoved: {
            FunctionDescription desc = diffItem.functionA();
            desc.diffItemIndex = i;
            fcnsA.append(desc);
            break;
        }
        case DiffItemAdded: {
            FunctionDescription desc = diffItem.functionB();
            desc.diffItemIndex = i;
            fcnsB.append(desc);
            break;
        }
        case DiffItemMatched: {
            FunctionDescription descA = diffItem.functionA();
            FunctionDescription descB = diffItem.functionB();
            descA.diffItemIndex = i;
            descB.diffItemIndex = i;
            fcnsA.append(descA);
            fcnsB.append(descB);
            break;
        }
        default: {
            break;
        }
        }
    }
    modelA->reload();
    modelB->reload();
}
