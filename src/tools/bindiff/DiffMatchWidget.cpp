#include "DiffMatchWidget.h"

#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <shortcuts/ShortcutManager.h>

DiffMatchProxyModel::DiffMatchProxyModel(QObject *parent) : QSortFilterProxyModel(parent) {}

void DiffMatchProxyModel::setHidePerfectMatches(bool hide)
{
    if (hidePerfectMatches == hide) {
        return;
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    beginFilterChange();
    hidePerfectMatches = hide;
    endFilterChange();
#else
    hidePerfectMatches = hide;
    invalidateFilter();
#endif
}

bool DiffMatchProxyModel::filterAcceptsRow(int sourceRow,
                                           const QModelIndex & /*sourceParent*/) const
{
    const auto *model = qobject_cast<const DiffMatchModel *>(sourceModel());

    if (!model) {
        return false;
    }
    if (hidePerfectMatches) {
        const QModelIndex similarityIndex = model->index(sourceRow, DiffMatchModel::Similarity);

        if (similarityIndex.data(Qt::UserRole).toDouble() >= 1.0) {
            return false;
        }
    }

    if (qhelpers::filterRegexEmpty(this)) {
        return true;
    }
    for (int column = 0; column < DiffMatchModel::ColumnCount; ++column) {

        const QModelIndex index = model->index(sourceRow, column);

        const QString text = index.data(Qt::DisplayRole).toString();

        if (qhelpers::filterStringContains(text, this)) {
            return true;
        }
    }

    return false;
}

bool DiffMatchProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    switch (left.column()) {
    case DiffMatchModel::NameOrig:
    case DiffMatchModel::NameMod:
        return left.data(Qt::DisplayRole)
                       .toString()
                       .compare(right.data(Qt::DisplayRole).toString(), Qt::CaseInsensitive)
                < 0;

    case DiffMatchModel::SizeOrig:
        return left.data(Qt::UserRole).toULongLong() < right.data(Qt::UserRole).toULongLong();

    case DiffMatchModel::AddressOrig:
        return left.data(Qt::UserRole).toULongLong() < right.data(Qt::UserRole).toULongLong();

    case DiffMatchModel::Similarity:
        return left.data(Qt::UserRole).toDouble() < right.data(Qt::UserRole).toDouble();

    case DiffMatchModel::AddressMod:
        return left.data(Qt::UserRole).toULongLong() < right.data(Qt::UserRole).toULongLong();

    case DiffMatchModel::SizeMod:
        return left.data(Qt::UserRole).toULongLong() < right.data(Qt::UserRole).toULongLong();

    default:
        return QSortFilterProxyModel::lessThan(left, right);
    }
}

DiffMatchModel::DiffMatchModel(QList<BinDiffMatchDescription> *list, QObject *parent)
    : QAbstractListModel(parent), list(list)
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
    case Qt::UserRole: {
        switch (index.column()) {
        case NameOrig:
            return entry.original.name;

        case SizeOrig:
            return QVariant::fromValue(entry.original.linearSize);

        case AddressOrig:
            return QVariant::fromValue(entry.original.offset);

        case Similarity:
            return entry.similarity;

        case AddressMod:
            return QVariant::fromValue(entry.modified.offset);

        case SizeMod:
            return QVariant::fromValue(entry.modified.linearSize);

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
    return QColor::fromRgbF(red, green, blue, .5);
}

void DiffMatchModel::reload()
{
    beginResetModel();
    perfect = Config()->getColor("gui.match.perfect");
    partial = Config()->getColor("gui.match.partial");
    endResetModel();
}

DiffMatchWidget::DiffMatchWidget(CutterDiff *cutterDiff, CutterDiffWindow *parent)
    : CutterDiffWidget(cutterDiff, parent),
      treeView(new CutterTreeView(this)),
      model(new DiffMatchModel(&list, this)),
      proxyModel(new DiffMatchProxyModel(this)),
      checkBoxHideIdentical(new QCheckBox(this)),
      matchesFilter(new QuickFilterView(this))
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    proxyModel->setSourceModel(model);

    treeView->setModel(proxyModel);
    treeView->setSortingEnabled(true);

    // QuickFilter Setup

    connect(matchesFilter, &QuickFilterView::filterTextChanged, proxyModel,
            &QSortFilterProxyModel::setFilterWildcard);

    QShortcut *searchShortcut = Shortcuts()->makeQShortcut("General.showFilter", treeView);
    searchShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    QShortcut *clearShortcut = Shortcuts()->makeQShortcut("General.clearFilter", matchesFilter);
    clearShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    connect(searchShortcut, &QShortcut::activated, matchesFilter, &QuickFilterView::showFilter);
    connect(clearShortcut, &QShortcut::activated, matchesFilter, &QuickFilterView::clearFilter);

    treeView->setIndentation(0);
    treeView->setItemsExpandable(false);
    treeView->setRootIsDecorated(false);
    auto bottomHBox = new QHBoxLayout();
    checkBoxHideIdentical->setText("Hide identical functions");
    checkBoxHideIdentical->setLayoutDirection(Qt::LeftToRight);
    bottomHBox->addStretch();
    bottomHBox->addWidget(checkBoxHideIdentical);
    setLayout(layout);
    layout->addWidget(treeView);
    layout->addLayout(bottomHBox);
    layout->addWidget(matchesFilter);
    connect(cutterDiff, &CutterDiff::diffDataUpdated, this, &DiffMatchWidget::reload);
    connect(treeView, &CutterTreeView::doubleClicked, this, [this](const QModelIndex &index) {
        this->diffWindow->showPrevMemoryWidget();
        const QModelIndex sourceIndex = proxyModel->mapToSource(index);

        this->cutterDiff->setCurrentDiffItemIndex(list[sourceIndex.row()].diffItemIndex);
    });

    connect(checkBoxHideIdentical, &QCheckBox::toggled, proxyModel,
            &DiffMatchProxyModel::setHidePerfectMatches);
    // context menu

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
                                            list[sourceIndex.row()].diffItemIndex,
                                            sourceIndex.column() < DiffMatchModel::AddressMod);
    });
}

void DiffMatchWidget::reload()
{
    list.clear();
    for (int i = 0; i < cutterDiff->getDiffItemList().size(); i++) {
        const CutterDiffItem &diffItem = cutterDiff->getDiffItemList()[i];
        if (diffItem.getType() == DiffItemMatched) {
            BinDiffMatchDescription desc = diffItem.toBinDiffMatchDescription();
            desc.diffItemIndex = i;
            list.push_back(desc);
        }
    }
    model->reload();
}
