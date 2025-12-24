#include "SegmentsWidget.h"
#include "core/MainWindow.h"
#include "common/Helpers.h"
#include "ui_ListDockWidget.h"

#include <QVBoxLayout>
#include <QShortcut>

SegmentsModel::SegmentsModel(QObject *parent) : AddressableItemModel<QAbstractListModel>(parent) {}

int SegmentsModel::rowCount(const QModelIndex &) const
{
    return segments.count();
}

int SegmentsModel::columnCount(const QModelIndex &) const
{
    return SegmentsModel::ColumnCount;
}

QVariant SegmentsModel::data(const QModelIndex &index, int role) const
{
    // TODO: create unique colors, e. g. use HSV color space and rotate in H for 360/size
    static const QList<QColor> colors = {
        QColor("#1ABC9C"), // TURQUOISE
        QColor("#2ECC71"), // EMERALD
        QColor("#3498DB"), // PETER RIVER
        QColor("#9B59B6"), // AMETHYST
        QColor("#34495E"), // WET ASPHALT
        QColor("#F1C40F"), // SUN FLOWER
        QColor("#E67E22"), // CARROT
        QColor("#E74C3C"), // ALIZARIN
        QColor("#ECF0F1"), // CLOUDS
        QColor("#BDC3C7"), // SILVER
        QColor("#95A5A6") // COBCRETE
    };

    if (index.row() >= segments.count())
        return QVariant();

    const SegmentDescription &segment = segments.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case SegmentsModel::NameColumn:
            return segment.name;
        case SegmentsModel::SizeColumn:
            return QString::number(segment.size);
        case SegmentsModel::AddressColumn:
            return RzAddressString(segment.vaddr);
        case SegmentsModel::EndAddressColumn:
            return RzAddressString(segment.vaddr + segment.size);
        case SegmentsModel::PermColumn:
            return segment.perm;
        case SegmentsModel::CommentColumn:
            return Core()->getCommentAt(segment.vaddr);
        default:
            return QVariant();
        }
    case Qt::DecorationRole:
        if (index.column() == 0)
            return colors[index.row() % colors.size()];
        return QVariant();
    case SegmentsModel::SegmentDescriptionRole:
        return QVariant::fromValue(segment);
    default:
        return QVariant();
    }
}

QVariant SegmentsModel::headerData(int segment, Qt::Orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        switch (segment) {
        case SegmentsModel::NameColumn:
            return tr("Name");
        case SegmentsModel::SizeColumn:
            return tr("Size");
        case SegmentsModel::AddressColumn:
            return tr("Address");
        case SegmentsModel::EndAddressColumn:
            return tr("End Address");
        case SegmentsModel::PermColumn:
            return tr("Permissions");
        case SegmentsModel::CommentColumn:
            return tr("Comment");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

RVA SegmentsModel::address(const QModelIndex &index) const
{
    const SegmentDescription &segment = segments.at(index.row());
    return segment.vaddr;
}

QString SegmentsModel::name(const QModelIndex &index) const
{
    const SegmentDescription &segment = segments.at(index.row());
    return segment.name;
}

SegmentsProxyModel::SegmentsProxyModel(SegmentsModel *sourceModel, QObject *parent)
    : AddressableFilterProxyModel(sourceModel, parent)
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

bool SegmentsProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    auto leftSegment = left.data(SegmentsModel::SegmentDescriptionRole).value<SegmentDescription>();
    auto rightSegment =
            right.data(SegmentsModel::SegmentDescriptionRole).value<SegmentDescription>();
    switch (left.column()) {
    case SegmentsModel::NameColumn:
        return leftSegment.name < rightSegment.name;
    case SegmentsModel::SizeColumn:
        return leftSegment.size < rightSegment.size;
    case SegmentsModel::AddressColumn:
    case SegmentsModel::EndAddressColumn:
        return leftSegment.vaddr < rightSegment.vaddr;
    case SegmentsModel::CommentColumn:
        return Core()->getCommentAt(leftSegment.vaddr) < Core()->getCommentAt(rightSegment.vaddr);
    default:
        break;
    }
    return false;
}

SegmentsWidget::SegmentsWidget(MainWindow *main) : ListDockWidget(main)
{
    setObjectName("SegmentsWidget");
    setWindowTitle(tr("Segments"));

    segmentsModel = new SegmentsModel(this);
    auto proxyModel = new SegmentsProxyModel(segmentsModel, this);
    setModels(proxyModel);

    ui->treeView->sortByColumn(SegmentsModel::NameColumn, Qt::AscendingOrder);

    ui->quickFilterView->closeFilter();
    showCount(false);

    connect(Core(), &CutterCore::refreshAll, this, &SegmentsWidget::refreshSegments);
    connect(Core(), &CutterCore::codeRebased, this, &SegmentsWidget::refreshSegments);
    connect(Core(), &CutterCore::commentsChanged, this,
            [this]() { qhelpers::emitColumnChanged(segmentsModel, SegmentsModel::CommentColumn); });
}

SegmentsWidget::~SegmentsWidget() {}

void SegmentsWidget::refreshSegments()
{
    segmentsModel->beginResetModel();
    segmentsModel->segments = Core()->getAllSegments();
    segmentsModel->endResetModel();

    qhelpers::adjustColumns(ui->treeView, SegmentsModel::ColumnCount, 0);
}
