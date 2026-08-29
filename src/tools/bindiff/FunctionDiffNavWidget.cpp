#include "FunctionDiffNavWidget.h"

#include <QSplitter>
#include <QVBoxLayout>

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
      modelB(new FunctionListModel(&fcnsB, this))
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

    splitter->addWidget(containerA);

    // Function container B
    auto containerB = new QWidget(this);
    auto vBoxB = new QVBoxLayout(containerB);
    vBoxB->setContentsMargins(0, 0, 0, 0);
    vBoxB->setSpacing(0);

    vBoxB->addWidget(labelFileB);
    vBoxB->addWidget(treeViewB);

    splitter->addWidget(containerB);

    treeViewA->setModel(modelA);
    treeViewB->setModel(modelB);

    connect(cutterDiff, &CutterDiff::diffDataUpdated, this, &FunctionDiffNavWidget::reload);
    connect(treeViewA, &CutterTreeView::doubleClicked, this, [this](const QModelIndex &index) {
        this->cutterDiff->setCurrentDiffItemIndex(fcnsA[index.row()].diffItemIndex);
    });
    connect(treeViewB, &CutterTreeView::doubleClicked, this, [this](const QModelIndex &index) {
        this->cutterDiff->setCurrentDiffItemIndex(fcnsB[index.row()].diffItemIndex);
    });

    reload();
}

void FunctionDiffNavWidget::reload()
{
    fcnsA.clear();
    fcnsB.clear();
    labelFileA->setText(cutterDiff->getFileName(true));
    labelFileB->setText(cutterDiff->getFileName(false));
    const QList<CutterDiffItem> &diffItemList = cutterDiff->getDiffItemList();
    for (int i = 0; i < diffItemList.size(); i++) {
        const CutterDiffItem &diffItem = diffItemList[i];
        const DiffItemType type = diffItem.getType();
        switch (type) {
        case DiffItemRemoved: {
            FunctionDescription desc = diffItem.functionA();
            desc.diffItemIndex = i;
            fcnsA.emplaceBack(desc);
            break;
        }
        case DiffItemAdded: {
            FunctionDescription desc = diffItem.functionB();
            desc.diffItemIndex = i;
            fcnsB.emplaceBack(desc);
            break;
        }
        case DiffItemMatched: {
            FunctionDescription descA = diffItem.functionA();
            FunctionDescription descB = diffItem.functionB();
            descA.diffItemIndex = i;
            descB.diffItemIndex = i;
            fcnsA.emplaceBack(descA);
            fcnsB.emplaceBack(descB);
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
