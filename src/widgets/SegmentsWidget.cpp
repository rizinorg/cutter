#include "SegmentsWidget.h"
#include "CutterTreeView.h"
#include "core/MainWindow.h"
#include "QuickFilterView.h"
#include "common/Helpers.h"

#include <QVBoxLayout>
#include <QShortcut>

SegmentsModel::SegmentsModel(QList<SegmentDescription> *segments, QObject *parent)
    : QAbstractListModel(parent),
      segments(segments)
{
}

int SegmentsModel::rowCount(const QModelIndex &) const
{
    return segments->count();
}

int SegmentsModel::columnCount(const QModelIndex &) const
{
    return SegmentsModel::ColumnCount;
}

QVariant SegmentsModel::data(const QModelIndex &index, int role) const
{
    // TODO: create unique colors, e. g. use HSV color space and rotate in H for 360/size
    static const QList<QColor> colors = { QColor("#1ABC9C"),    //TURQUOISE
                                          QColor("#2ECC71"),    //EMERALD
                                          QColor("#3498DB"),    //PETER RIVER
                                          QColor("#9B59B6"),    //AMETHYST
                                          QColor("#34495E"),    //WET ASPHALT
                                          QColor("#F1C40F"),    //SUN FLOWER
                                          QColor("#E67E22"),    //CARROT
                                          QColor("#E74C3C"),    //ALIZARIN
                                          QColor("#ECF0F1"),    //CLOUDS
                                          QColor("#BDC3C7"),    //SILVER
                                          QColor("#95A5A6")     //COBCRETE
                                        };

    if (index.row() >= segments->count())
        return QVariant();

    const SegmentDescription &segment = segments->at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case SegmentsModel::NameColumn:
            return segment.name;
        case SegmentsModel::SizeColumn:
            return segment.size;
        case SegmentsModel::AddressColumn:
            return RAddressString(segment.vaddr);
        case SegmentsModel::EndAddressColumn:
            return RAddressString(segment.vaddr + segment.size);
        case SegmentsModel::PermColumn:
            return segment.perm;
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
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

SegmentsProxyModel::SegmentsProxyModel(SegmentsModel *sourceModel, QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(sourceModel);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

bool SegmentsProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    auto leftSegment = left.data(SegmentsModel::SegmentDescriptionRole).value<SegmentDescription>();
    auto rightSegment = right.data(SegmentsModel::SegmentDescriptionRole).value<SegmentDescription>();
    switch (left.column()) {
    case SegmentsModel::NameColumn:
        return leftSegment.name < rightSegment.name;
    case SegmentsModel::SizeColumn:
        return leftSegment.size < rightSegment.size;
    case SegmentsModel::AddressColumn:
    case SegmentsModel::EndAddressColumn:
        return leftSegment.vaddr < rightSegment.vaddr;
    default:
        break;
    }
    return false;
}

SegmentsWidget::SegmentsWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    main(main)
{

    setObjectName("SegmentsWidget");
    setWindowTitle(QStringLiteral("Segments"));

    segmentsTable = new CutterTreeView;
    segmentsModel = new SegmentsModel(&segments, this);
    auto proxyModel = new SegmentsProxyModel(segmentsModel, this);

    segmentsTable->setModel(proxyModel);
    segmentsTable->setIndentation(10);
    segmentsTable->setSortingEnabled(true);
    segmentsTable->sortByColumn(SegmentsModel::NameColumn, Qt::AscendingOrder);

    connect(segmentsTable, SIGNAL(doubleClicked(const QModelIndex &)),
            this, SLOT(onSegmentsDoubleClicked(const QModelIndex &)));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    connect(Core(), SIGNAL(refreshAll()), this, SLOT(refreshSegments()));

    quickFilterView = new QuickFilterView(this, false);
    quickFilterView->setObjectName(QStringLiteral("quickFilterView"));
    QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Maximum);
    sizePolicy1.setHorizontalStretch(0);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(quickFilterView->sizePolicy().hasHeightForWidth());
    quickFilterView->setSizePolicy(sizePolicy1);

    QShortcut *search_shortcut = new QShortcut(QKeySequence::Find, this);
    connect(search_shortcut, &QShortcut::activated, quickFilterView, &QuickFilterView::showFilter);
    search_shortcut->setContext(Qt::WidgetWithChildrenShortcut);

    QShortcut *clear_shortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(clear_shortcut, &QShortcut::activated, quickFilterView, &QuickFilterView::clearFilter);
    clear_shortcut->setContext(Qt::WidgetWithChildrenShortcut);

    connect(quickFilterView, SIGNAL(filterTextChanged(const QString &)), proxyModel,
            SLOT(setFilterWildcard(const QString &)));
    connect(quickFilterView, SIGNAL(filterClosed()), segmentsTable, SLOT(setFocus()));

    dockWidgetContents = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(segmentsTable);
    layout->addWidget(quickFilterView);
    layout->setMargin(0);
    dockWidgetContents->setLayout(layout);
    setWidget(dockWidgetContents);
}

SegmentsWidget::~SegmentsWidget() {}

void SegmentsWidget::refreshSegments()
{
    segmentsModel->beginResetModel();
    segments = Core()->getAllSegments();
    segmentsModel->endResetModel();

    qhelpers::adjustColumns(segmentsTable, SegmentsModel::ColumnCount, 0);
}

void SegmentsWidget::onSegmentsDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    auto segment = index.data(SegmentsModel::SegmentDescriptionRole).value<SegmentDescription>();
    Core()->seek(segment.vaddr);
}
