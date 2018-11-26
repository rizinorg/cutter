#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "common/Configuration.h"
#include "SectionsWidget.h"
#include "CutterTreeView.h"
#include "MainWindow.h"
#include "QuickFilterView.h"
#include "common/Helpers.h"
#include "common/Configuration.h"

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

    sectionsTable = new CutterTreeView;
    sectionsModel = new SectionsModel(&sections, this);
    auto proxyModel = new SectionsProxyModel(sectionsModel, this);

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
    rawAddrDock = new SectionAddrDock(sectionsModel, SectionAddrDock::Raw, this);
    virtualAddrDock = new SectionAddrDock(sectionsModel, SectionAddrDock::Virtual, this);

    addrDockWidget = new QWidget();
    QHBoxLayout *addrDockLayout = new QHBoxLayout();
    addrDockLayout->addWidget(rawAddrDock);
    addrDockLayout->addWidget(virtualAddrDock);
    addrDockWidget->setLayout(addrDockLayout);
    layout->addWidget(addrDockWidget);

    QPixmap map(":/img/icons/previous.svg");
    QTransform transform;
    transform = transform.rotate(90);
    map = map.transformed(transform);
    QIcon icon;
    icon.addPixmap(map);

    toggleButton = new QToolButton;
    toggleButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    toggleButton->setFixedHeight(30);
    toggleButton->setIcon(icon);
    toggleButton->setIconSize(QSize(16, 12));
    toggleButton->setAutoRaise(true);
    toggleButton->setArrowType(Qt::NoArrow);
    toggleButton->hide();
    layout->addWidget(toggleButton);

    layout->setMargin(0);
    dockWidgetContents->setLayout(layout);
    setWidget(dockWidgetContents);

    connect(this, &QDockWidget::visibilityChanged, this, [ = ](bool visibility) {
        if (visibility) {
            refreshSections();
        }
    });
    connect(Core(), SIGNAL(seekChanged(RVA)), this, SLOT(onSectionsSeekChanged(RVA)));
    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(refreshSections()));
    connect(toggleButton, &QToolButton::clicked, this, [ = ] {
        toggleButton->hide();
        addrDockWidget->show();
        rawAddrDock->show();
        virtualAddrDock->show();
    });

    indicatorWidth = 600;
    indicatorHeight = 5;
    indicatorParamPosY = 20;

    connect(rawAddrDock, &QDockWidget::visibilityChanged, this, [ = ](bool visibility) {
        if (!visibility) {
            updateToggle();
        }
    });
    connect(virtualAddrDock, &QDockWidget::visibilityChanged, this, [ = ](bool visibility) {
        if (!visibility) {
            updateToggle();
        }
    });
}

SectionsWidget::~SectionsWidget() {}

void SectionsWidget::refreshSections()
{
    sectionsModel->beginResetModel();
    sections = Core()->getAllSections();
    sectionsModel->endResetModel();

    qhelpers::adjustColumns(sectionsTable, SectionsModel::ColumnCount, 0);
    rawAddrDock->show();
    virtualAddrDock->show();
    rawAddrDock->updateDock();
    virtualAddrDock->updateDock();
    drawIndicatorOnAddrDocks();
}

void SectionsWidget::onSectionsDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    auto section = index.data(SectionsModel::SectionDescriptionRole).value<SectionDescription>();
    Core()->seek(section.vaddr);
}

void SectionsWidget::onSectionsSeekChanged(RVA addr)
{
    Q_UNUSED(addr);
    rawAddrDock->updateDock();
    virtualAddrDock->updateDock();
    drawIndicatorOnAddrDocks();
}

void SectionsWidget::drawIndicatorOnAddrDocks()
{
    RVA offset = Core()->getOffset();
    for (int i = 0; i != virtualAddrDock->proxyModel->rowCount(); i++) {
        QModelIndex idx = virtualAddrDock->proxyModel->index(i, 0);
        RVA vaddr = idx.data(SectionsModel::SectionDescriptionRole).value<SectionDescription>().vaddr;
        int vsize = idx.data(SectionsModel::SectionDescriptionRole).value<SectionDescription>().vsize;
        RVA end = vaddr + vsize;
        if (offset < end) {
            QString name = idx.data(SectionsModel::SectionDescriptionRole).value<SectionDescription>().name;
            float ratio = 0;
            if (vsize > 0 && offset > vaddr) {
                ratio = (float)(offset - vaddr) / (float)vsize;
            }
            updateIndicator(rawAddrDock, name, ratio);
            updateIndicator(virtualAddrDock, name, ratio);
            return;
        }
    }
}

void SectionsWidget::updateIndicator(SectionAddrDock *targetDock, QString name, float ratio)
{
    RVA offset = Core()->getOffset();
    float padding = targetDock->nameHeightMap[name] * ratio;
    int y = targetDock->namePosYMap[name] + (int)padding;
    QColor color = targetDock->indicatorColor;
    QGraphicsRectItem *indicator = new QGraphicsRectItem(QRectF(0, y, indicatorWidth, indicatorHeight));
    indicator->setBrush(QBrush(color));
    targetDock->graphicsScene->addItem(indicator);
    targetDock->graphicsView->centerOn(indicator);

    targetDock->addTextItem(color, QPoint(targetDock->rectOffset + targetDock->rectWidth, y - indicatorParamPosY), name);
    targetDock->addTextItem(color, QPoint(0, y - indicatorParamPosY), QString("0x%1").arg(offset, 0, 16));
}

void SectionsWidget::updateToggle()
{
    if (!rawAddrDock->isVisible() && !virtualAddrDock->isVisible()) {
        addrDockWidget->hide();
        toggleButton->show();
    }
}

SectionAddrDock::SectionAddrDock(SectionsModel *model, AddrType type, QWidget *parent) :
    QDockWidget(parent),
    graphicsScene(new QGraphicsScene),
    graphicsView(new QGraphicsView)
{
    setStyleSheet(QString("color:%1;").arg(ConfigColor("gui.dataoffset").name()));
    switch (type) {
        case SectionAddrDock::Raw:
            setWindowTitle(tr("Raw"));
            break;
        case SectionAddrDock::Virtual:
            setWindowTitle(tr("Virtual"));
            break;
        default:
            return;
    }

    graphicsView->setScene(graphicsScene);
    setWidget(graphicsView);
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    proxyModel = new SectionsProxyModel(model, this);
    addrType = type;

    QWidget *w = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(graphicsView);
    w->setLayout(layout);
    setWidget(w);

    heightThreshold = 30;
    rectOffset = 100;
    rectWidth = 400;
    indicatorColor = ConfigColor("gui.navbar.err");

    connect(this, &QDockWidget::featuresChanged, this, [ = ](){
        setFeatures(QDockWidget::DockWidgetClosable);
    });
}

void SectionAddrDock::updateDock()
{
    setFeatures(QDockWidget::DockWidgetClosable);

    graphicsScene->clear();

    setStyleSheet(QString("color:%1;").arg(ConfigColor("gui.dataoffset").name()));
    const QBrush bg = QBrush(ConfigColor("gui.background"));
    graphicsScene->setBackgroundBrush(bg);

    int y = 0;
    proxyModel->sort(2, Qt::AscendingOrder);
    for (int i = 0; i < proxyModel->rowCount(); i++) {
        QModelIndex idx = proxyModel->index(i, 0);
        RVA addr;
        int size;
        switch (addrType) {
            case SectionAddrDock::Raw:
                addr = idx.data(SectionsModel::SectionDescriptionRole).value<SectionDescription>().paddr;
                size = idx.data(SectionsModel::SectionDescriptionRole).value<SectionDescription>().size;
                break;
            case SectionAddrDock::Virtual:
                addr = idx.data(SectionsModel::SectionDescriptionRole).value<SectionDescription>().vaddr;
                size = idx.data(SectionsModel::SectionDescriptionRole).value<SectionDescription>().vsize;
                break;
            default:
                return;
        }
        QString name = idx.data(SectionsModel::SectionDescriptionRole).value<SectionDescription>().name;
        if (size < heightThreshold) {
            size = heightThreshold;
        } else {
            size /= heightThreshold;
            size = std::max(size, heightThreshold);
        }
        QGraphicsRectItem *rect = new QGraphicsRectItem(rectOffset, y, rectWidth, size);
        rect->setBrush(QBrush(idx.data(Qt::DecorationRole).value<QColor>()));
        graphicsScene->addItem(rect);

        addTextItem(ConfigColor("gui.dataoffset"), QPoint(0, y), QString("0x%1").arg(addr, 0, 16));
        addTextItem(ConfigColor("gui.dataoffset"), QPoint(rectOffset, y), QString::number(size));
        addTextItem(ConfigColor("gui.dataoffset"), QPoint(rectOffset + rectWidth, y), name);

        namePosYMap[name] = y;
        nameHeightMap[name] = size;

        y += size;
    }
}

void SectionAddrDock::addTextItem(QColor color, QPoint pos, QString string)
{
    QGraphicsTextItem *text = new QGraphicsTextItem;
    text->setDefaultTextColor(color);
    text->setPos(pos);
    text->setPlainText(string);
    graphicsScene->addItem(text);
}
