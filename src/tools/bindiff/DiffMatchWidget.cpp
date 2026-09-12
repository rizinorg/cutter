#include "DiffMatchWidget.h"

#include <QApplication>
#include <QClipboard>
#include <QVBoxLayout>

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
      model(new DiffMatchModel(&list, this))
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
    connect(cutterDiff, &CutterDiff::diffDataUpdated, this, &DiffMatchWidget::reload);
    connect(treeView, &CutterTreeView::doubleClicked, this, [this](const QModelIndex &index) {
        this->cutterDiff->setCurrentDiffItemIndex(list[index.row()].diffItemIndex);
    });

    // context menu

    treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(treeView, &QTreeView::customContextMenuRequested, this,
            &DiffMatchWidget::showContextMenu);
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

void DiffMatchWidget::showContextMenu(const QPoint &pos)
{
    const QModelIndex index = treeView->indexAt(pos);
    const QPair<RVA, RVA> addr = model->address(index);

    QMenu menu(this);

    const QAction *seekTo = menu.addAction("Seek to");
    const QAction *goToAndAlign = menu.addAction("Align HexDiff");
    const QAction *diffFunctionLines = menu.addAction("Line Diff");
    const QAction *copyAddress = menu.addAction("Copy Address");
    const QAction *showGraphDiff = menu.addAction("Graph Diff");

    const QAction *selected = menu.exec(treeView->viewport()->mapToGlobal(pos));

    cutterDiff->setCurrentDiffItemIndex(list[index.row()].diffItemIndex);

    if (selected == seekTo) {
        if (index.column() < DiffMatchModel::AddressMod) {
            diffWindow->seekAndShowHexDiff({ addr.first, RVA_INVALID });
        } else {
            diffWindow->seekAndShowHexDiff({ RVA_INVALID, addr.second });
        }
    } else if (selected == goToAndAlign) {
        diffWindow->seekAndShowHexDiff(addr);
    } else if (selected == diffFunctionLines) {
        diffWindow->showLineDiff();
    } else if (selected == copyAddress) {
        if (index.column() < DiffMatchModel::AddressMod) {
            QApplication::clipboard()->setText(rzAddressString(addr.first));
        } else {
            QApplication::clipboard()->setText(rzAddressString(addr.second));
        }
    } else if (selected == showGraphDiff) {
        diffWindow->showGraphDiff();
    }
}
