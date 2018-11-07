#include <QTreeView>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPen>

#include "SectionsWidget.h"
#include "MainWindow.h"
#include "QuickFilterView.h"
#include "common/Helpers.h"

SectionsModel::SectionsModel(QList<SectionDescription> *sections, QObject *parent)
    : QAbstractListModel(parent),
      sections(sections)
{
}

int SectionsModel::rowCount(const QModelIndex &) const
{
    return sections->count();
}

int SectionsModel::columnCount(const QModelIndex &) const
{
    return SectionsModel::ColumnCount;
}

QVariant SectionsModel::data(const QModelIndex &index, int role) const
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

    if (index.row() >= sections->count())
        return QVariant();

    const SectionDescription &section = sections->at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case SectionsModel::NameColumn:
            return section.name;
        case SectionsModel::SizeColumn:
            return section.vsize;
        case SectionsModel::AddressColumn:
            return RAddressString(section.vaddr);
        case SectionsModel::EndAddressColumn:
            return RAddressString(section.vaddr + section.vsize);
        case SectionsModel::EntropyColumn:
            return section.entropy;
        default:
            return QVariant();
        }
    case Qt::DecorationRole:
        if (index.column() == 0)
            return colors[index.row() % colors.size()];
        return QVariant();
    case SectionsModel::SectionDescriptionRole:
        return QVariant::fromValue(section);
    default:
        return QVariant();
    }
}

QVariant SectionsModel::headerData(int section, Qt::Orientation, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case SectionsModel::NameColumn:
            return tr("Name");
        case SectionsModel::SizeColumn:
            return tr("Virtual Size");
        case SectionsModel::AddressColumn:
            return tr("Address");
        case SectionsModel::EndAddressColumn:
            return tr("End Address");
        case SectionsModel::EntropyColumn:
            return tr("Entropy");
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

QColor SectionsModel::getColor(int colorIndex)
{
    QModelIndex i = index(colorIndex, 0);
    return i.data(Qt::DecorationRole).value<QColor>();
}

SectionDescription SectionsModel::getSectionDescription(int stringIndex)
{
    QModelIndex i = index(stringIndex, 0);
    return i.data(SectionsModel::SectionDescriptionRole).value<SectionDescription>();
}

SectionsProxyModel::SectionsProxyModel(SectionsModel *sourceModel, QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(sourceModel);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

bool SectionsProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    auto leftSection = left.data(SectionsModel::SectionDescriptionRole).value<SectionDescription>();
    auto rightSection = right.data(SectionsModel::SectionDescriptionRole).value<SectionDescription>();

    switch (left.column()) {
    case SectionsModel::NameColumn:
        return leftSection.name < rightSection.name;
    case SectionsModel::SizeColumn:
        return leftSection.vsize < rightSection.vsize;
    case SectionsModel::AddressColumn:
    case SectionsModel::EndAddressColumn:
        if (leftSection.vaddr != rightSection.vaddr) {
            return leftSection.vaddr < rightSection.vaddr;
        }
        return leftSection.vsize < rightSection.vsize;
    case SectionsModel::EntropyColumn:
        return leftSection.entropy < rightSection.entropy;

    default:
        break;
    }

    return false;
}

SectionsWidget::SectionsWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    main(main)
{
    setObjectName("SectionsWidget");
    setWindowTitle(QStringLiteral("Sections"));

    sectionsTable = new QTreeView;
    sectionsModel = new SectionsModel(&sections, this);
    auto proxyModel = new SectionsProxyModel(sectionsModel, this);

    rawAddrDock = new SectionAddrDock(sectionsModel, SectionAddrDock::Raw, this);
    virtualAddrDock = new SectionAddrDock(sectionsModel, SectionAddrDock::Virtual, this);

    sectionsTable->setModel(proxyModel);
    sectionsTable->setIndentation(10);
    sectionsTable->setSortingEnabled(true);
    sectionsTable->sortByColumn(SectionsModel::NameColumn, Qt::AscendingOrder);

    connect(sectionsTable, SIGNAL(doubleClicked(const QModelIndex &)),
            this, SLOT(onSectionsDoubleClicked(const QModelIndex &)));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    connect(Core(), SIGNAL(refreshAll()), this, SLOT(refreshSections()));
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
    connect(quickFilterView, SIGNAL(filterClosed()), sectionsTable, SLOT(setFocus()));

    dockWidgetContents = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(sectionsTable);
    layout->addWidget(quickFilterView);
    QWidget *hw = new QWidget();
    QHBoxLayout *layouth = new QHBoxLayout();
    layouth->addWidget(rawAddrDock);
    layouth->addWidget(virtualAddrDock);
    hw->setLayout(layouth);
    layout->addWidget(hw);
    layout->setMargin(0);
    dockWidgetContents->setLayout(layout);
    setWidget(dockWidgetContents);
    connect(sectionsTable->model(), SIGNAL(layoutChanged()), rawAddrDock, SLOT(updateDock()));
    connect(sectionsTable->model(), SIGNAL(layoutChanged()), virtualAddrDock, SLOT(updateDock()));
}

SectionsWidget::~SectionsWidget() {}

void SectionsWidget::refreshSections()
{
    sectionsModel->beginResetModel();
    sections = Core()->getAllSections();
    sectionsModel->endResetModel();

    qhelpers::adjustColumns(sectionsTable, SectionsModel::ColumnCount, 0);
    rawAddrDock->updateDock();
    virtualAddrDock->updateDock();
}

void SectionsWidget::onSectionsDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    auto section = index.data(SectionsModel::SectionDescriptionRole).value<SectionDescription>();
    Core()->seek(section.vaddr);
    //auto color = index.data(Qt::DecorationRole).value<QColor>();
    //eprintf ("%s\n", color.name().toUtf8().constData());
}

SectionAddrDock::SectionAddrDock(SectionsModel *model, AddrType type,  QWidget *parent) :
    QDockWidget(parent),
    graphicsView(new QGraphicsView),
    graphicsScene(new QGraphicsScene)
{
    const QBrush bg = QBrush(ConfigColor("gui.background"));
    graphicsScene = new QGraphicsScene(this);
    graphicsScene->setBackgroundBrush(bg);
    graphicsView->setScene(graphicsScene);
    setWidget(graphicsView);
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    proxyModel = new SectionsProxyModel(model, this);
    addrType = type;
}

void SectionAddrDock::updateDock()
{
    const int threshold = 30;
    int y = 0;
    int n = proxyModel->rowCount();
    proxyModel->sort(2, Qt::AscendingOrder);
    for (int i = 0; i < n; i++) {
        QModelIndex idx = proxyModel->index(i, 0);
        RVA t;
        int s;
        switch (addrType) {
            case SectionAddrDock::Raw:
                t = idx.data(SectionsModel::SectionDescriptionRole).value<SectionDescription>().paddr;
                s = idx.data(SectionsModel::SectionDescriptionRole).value<SectionDescription>().size;
                break;
            case SectionAddrDock::Virtual:
                t = idx.data(SectionsModel::SectionDescriptionRole).value<SectionDescription>().vaddr;
                s = idx.data(SectionsModel::SectionDescriptionRole).value<SectionDescription>().vsize;
                break;
            default:
                return;
        }
        QString t2 = idx.data(SectionsModel::SectionDescriptionRole).value<SectionDescription>().name;
        if (s < threshold) {
            s = threshold;
        } else {
            s /= threshold;
            s = std::max(s, threshold);
        }
        QGraphicsRectItem *item = new QGraphicsRectItem(100, y, 400, s);
        QGraphicsTextItem *text = new QGraphicsTextItem;
        //text->setDefaultTextColor(ConfigColor("btext"));
        text->setDefaultTextColor(Qt::white);
        QGraphicsTextItem *text2 = new QGraphicsTextItem;
        text2->setDefaultTextColor(Qt::white);
        text->setPos(0, y);
        text->setPlainText(QString("0x%1").arg(t, 0, 16));
        text2->setPos(500, y);
        text2->setPlainText(t2);
        item->setBrush(QBrush(idx.data(Qt::DecorationRole).value<QColor>()));
        graphicsScene->addItem(text);
        graphicsScene->addItem(item);
        graphicsScene->addItem(text2);
        y += s;
    }
    //for (int i = 0; i < n; i++) {
    //    QModelIndex idx = proxyModel->index(i, 0);
    //    eprintf ("%x\n", idx.data(SectionsModel::SectionDescriptionRole).value<SectionDescription>().vaddr + idx.data(SectionsModel::SectionDescriptionRole).value<SectionDescription>().vsize);
    //}
}

void SectionAddrDock::drawCursor()
{
}
