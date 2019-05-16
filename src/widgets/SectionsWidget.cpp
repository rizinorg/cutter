#include "SectionsWidget.h"
#include "CutterTreeView.h"
#include "QuickFilterView.h"
#include "core/MainWindow.h"
#include "common/Helpers.h"
#include "common/Configuration.h"

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QShortcut>
#include <QToolTip>

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

    if (index.row() >= sections->count()) {
        return QVariant();
    }

    const SectionDescription &section = sections->at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case SectionsModel::NameColumn:
            return section.name;
        case SectionsModel::SizeColumn:
            return RSizeString(section.vsize);
        case SectionsModel::AddressColumn:
            return RAddressString(section.vaddr);
        case SectionsModel::EndAddressColumn:
            return RAddressString(section.vaddr + section.vsize);
        case SectionsModel::PermissionsColumn:
            return section.perm;
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
        case SectionsModel::PermissionsColumn:
            return tr("Permissions");
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
    default:
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
    case SectionsModel::PermissionsColumn:
        return leftSection.perm < rightSection.perm;
    case SectionsModel::EntropyColumn:
        return leftSection.entropy < rightSection.entropy;
    }
}

SectionsWidget::SectionsWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    main(main)
{
    setObjectName("SectionsWidget");
    setWindowTitle(QStringLiteral("Sections"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    sectionsRefreshDeferrer = createRefreshDeferrer([this]() { refreshSections(); });
    dockRefreshDeferrer = createRefreshDeferrer([this]() { refreshDocks(); });

    initSectionsTable();
    initQuickFilter();
    initAddrMapDocks();
    initConnects();
}

SectionsWidget::~SectionsWidget() = default;

void SectionsWidget::initSectionsTable()
{
    sectionsTable = new CutterTreeView;
    sectionsModel = new SectionsModel(&sections, this);
    proxyModel = new SectionsProxyModel(sectionsModel, this);

    sectionsTable->setModel(proxyModel);
    sectionsTable->setIndentation(10);
    sectionsTable->setSortingEnabled(true);
    sectionsTable->sortByColumn(SectionsModel::NameColumn, Qt::AscendingOrder);
}

void SectionsWidget::initQuickFilter()
{
    quickFilterView = new QuickFilterView(this, false);
    quickFilterView->setObjectName(QStringLiteral("quickFilterView"));
    QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Maximum);
    sizePolicy1.setHorizontalStretch(0);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(quickFilterView->sizePolicy().hasHeightForWidth());
    quickFilterView->setSizePolicy(sizePolicy1);

    QShortcut *search_shortcut = new QShortcut(QKeySequence::Find, this);
    search_shortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(search_shortcut, &QShortcut::activated, quickFilterView, &QuickFilterView::showFilter);

    QShortcut *clear_shortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    clear_shortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(clear_shortcut, &QShortcut::activated, quickFilterView, &QuickFilterView::clearFilter);
}

void SectionsWidget::initAddrMapDocks()
{
    dockWidgetContents = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout();

    layout->addWidget(sectionsTable);
    layout->addWidget(quickFilterView);

    rawAddrDock = new RawAddrDock(sectionsModel, this);
    virtualAddrDock = new VirtualAddrDock(sectionsModel, this);
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
}

void SectionsWidget::initConnects()
{
    connect(sectionsTable, SIGNAL(doubleClicked(const QModelIndex &)),
            this, SLOT(onSectionsDoubleClicked(const QModelIndex &)));
    connect(Core(), SIGNAL(refreshAll()), this, SLOT(refreshSections()));
    connect(quickFilterView, SIGNAL(filterTextChanged(const QString &)), proxyModel,
            SLOT(setFilterWildcard(const QString &)));
    connect(quickFilterView, SIGNAL(filterClosed()), sectionsTable, SLOT(setFocus()));
    connect(this, &QDockWidget::visibilityChanged, this, [ = ](bool visibility) {
        if (visibility) {
            refreshSections();
        }
    });
    connect(Core(), &CutterCore::seekChanged, this, &SectionsWidget::refreshDocks);
    connect(Config(), &Configuration::colorsUpdated, this, &SectionsWidget::refreshSections);
    connect(toggleButton, &QToolButton::clicked, this, [ = ] {
        toggleButton->hide();
        addrDockWidget->show();
        virtualAddrDock->show();
    });
    connect(virtualAddrDock, &QDockWidget::visibilityChanged, this, [ = ](bool visibility) {
        if (!visibility) {
            updateToggle();
        }
    });
}

void SectionsWidget::refreshSections()
{
    if (!sectionsRefreshDeferrer->attemptRefresh(nullptr)) {
        return;
    }
    sectionsModel->beginResetModel();
    sections = Core()->getAllSections();
    sectionsModel->endResetModel();
    qhelpers::adjustColumns(sectionsTable, SectionsModel::ColumnCount, 0);
    refreshDocks();
}

void SectionsWidget::refreshDocks()
{
    if (!dockRefreshDeferrer->attemptRefresh(nullptr)) {
        return;
    }
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
            rawAddrDock->drawIndicator(name, ratio);
            virtualAddrDock->drawIndicator(name, ratio);
            return;
        }
    }
}

void SectionsWidget::onSectionsDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    auto section = index.data(SectionsModel::SectionDescriptionRole).value<SectionDescription>();
    Core()->seek(section.vaddr);
}

void SectionsWidget::resizeEvent(QResizeEvent *event) {
    CutterDockWidget::resizeEvent(event);
    refreshDocks();
}

void SectionsWidget::updateToggle()
{
    if (!virtualAddrDock->isVisible()) {
        addrDockWidget->hide();
        toggleButton->show();
    }
}

AbstractAddrDock::AbstractAddrDock(SectionsModel *model, QWidget *parent) :
    QDockWidget(parent),
    addrDockScene(new AddrDockScene),
    graphicsView(new QGraphicsView)
{
    graphicsView->setScene(addrDockScene);
    setWidget(graphicsView);
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    proxyModel = new SectionsProxyModel(model, this);

    setWidget(graphicsView);

    indicatorHeight = 5;
    indicatorParamPosY = 20;
    heightThreshold = 30;
    heightDivisor = 1000;
    rectOffset = 100;
    rectWidthMin = 80;
    rectWidthMax = 400;
    indicatorColor = ConfigColor("gui.navbar.seek");
    textColor = ConfigColor("gui.dataoffset");
}

AbstractAddrDock::~AbstractAddrDock() {}

void AbstractAddrDock::updateDock()
{
    addrDockScene->clear();

    const QBrush bg = QBrush(ConfigColor("gui.background"));
    addrDockScene->setBackgroundBrush(bg);

    textColor = ConfigColor("gui.dataoffset");

    int y = 0;
    int validMinSize = getValidMinSize();
    int rectWidth = getRectWidth();
    proxyModel->sort(SectionsModel::AddressColumn, Qt::AscendingOrder);
    for (int i = 0; i < proxyModel->rowCount(); ++i) {
        QModelIndex idx = proxyModel->index(i, 0);
        auto desc = idx.data(SectionsModel::SectionDescriptionRole).value<SectionDescription>();

        QString name = desc.name;

        addrDockScene->seekAddrMap[name] = desc.vaddr;
        addrDockScene->seekAddrSizeMap[name] = desc.vsize;

        RVA addr = getAddressOfSection(desc);
        RVA size = getSizeOfSection(desc);
        addrDockScene->nameAddrMap[name] = addr;
        addrDockScene->nameAddrSizeMap[name] = size;

        int drawSize = getAdjustedSize(size, validMinSize);

        QGraphicsRectItem *rect = new QGraphicsRectItem(rectOffset, y, rectWidth, drawSize);
        rect->setBrush(QBrush(idx.data(Qt::DecorationRole).value<QColor>()));
        addrDockScene->addItem(rect);

        addTextItem(textColor, QPoint(0, y), RAddressString(addr));
        addTextItem(textColor, QPoint(rectOffset, y), RSizeString(size));
        addTextItem(textColor, QPoint(rectOffset + rectWidth, y), name);

        addrDockScene->namePosYMap[name] = y;
        addrDockScene->nameHeightMap[name] = drawSize;

        y += drawSize;
    }

    graphicsView->setSceneRect(addrDockScene->itemsBoundingRect());
}

void AbstractAddrDock::addTextItem(QColor color, QPoint pos, QString string)
{
    QGraphicsTextItem *text = new QGraphicsTextItem;
    text->setDefaultTextColor(color);
    text->setPos(pos);
    text->setPlainText(string);
    addrDockScene->addItem(text);
}

int AbstractAddrDock::getAdjustedSize(int size, int validMinSize)
{
    if (size == 0) {
        return size;
    }
    if (size == validMinSize) {
        return heightThreshold;
    }
    float r = (float)size / (float)validMinSize;
    r /= heightDivisor;
    r += 1;
    return heightThreshold * r;
}

int AbstractAddrDock::getRectWidth()
{
    return qBound(rectWidthMin, width() - 300, rectWidthMax);
}

int AbstractAddrDock::getIndicatorWidth()
{
    return getRectWidth() + 200;
}

int AbstractAddrDock::getValidMinSize()
{
    proxyModel->sort(SectionsModel::SizeColumn, Qt::AscendingOrder);
    for (int i = 0; i < proxyModel->rowCount(); i++) {
        QModelIndex idx = proxyModel->index(i, 0);
        int size = getSizeOfSection(idx.data(SectionsModel::SectionDescriptionRole).value<SectionDescription>());
        if (size > 0) {
            return size;
        }
    }
    return 0;
}

void AbstractAddrDock::drawIndicator(QString name, float ratio)
{
    RVA offset = Core()->getOffset();
    float padding = addrDockScene->nameHeightMap[name] * ratio;
    int y = addrDockScene->namePosYMap[name] + (int)padding;
    QColor color = indicatorColor;
    QGraphicsRectItem *indicator = new QGraphicsRectItem(QRectF(0, y, getIndicatorWidth(), indicatorHeight));
    indicator->setBrush(QBrush(color));
    addrDockScene->addItem(indicator);

    if (!addrDockScene->disableCenterOn) {
        graphicsView->centerOn(indicator);
    }

    addTextItem(color, QPoint(rectOffset + getRectWidth(), y - indicatorParamPosY), name);
    addTextItem(color, QPoint(0, y - indicatorParamPosY), QString("0x%1").arg(offset, 0, 16));
}

AddrDockScene::AddrDockScene(QWidget *parent) :
    QGraphicsScene(parent)
{
    disableCenterOn = false;
}

AddrDockScene::~AddrDockScene() {}

void AddrDockScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    RVA addr = getAddrFromPos((int)event->scenePos().y(), false);
    if (addr != RVA_INVALID) {
        QToolTip::showText(event->screenPos(), RAddressString(addr));
        if (event->buttons() & Qt::LeftButton) {
            RVA seekAddr = getAddrFromPos((int)event->scenePos().y(), true);
            disableCenterOn = true;
            Core()->seek(seekAddr);
            disableCenterOn = false;
            return;
        }
    } else {
        QToolTip::hideText();
    }
}

void AddrDockScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    mousePressEvent(event);
}

RVA AddrDockScene::getAddrFromPos(int posY, bool seek)
{
    QHash<QString, int>::const_iterator it;
    QHash<QString, RVA> addrMap = seek ? seekAddrMap : nameAddrMap;
    QHash<QString, RVA> addrSizeMap = seek ? seekAddrSizeMap : nameAddrSizeMap;
    for (it = namePosYMap.constBegin(); it != namePosYMap.constEnd(); ++it) {
        QString name = it.key();
        int y = it.value();
        int h = nameHeightMap[name];
        if (posY >= y && y + h >= posY) {
            if (h == 0) {
                return addrMap[name];
            }
            return addrMap[name] + (float)addrSizeMap[name] * ((float)(posY - y) / (float)h);
        }
    }
    return RVA_INVALID;
}

RawAddrDock::RawAddrDock(SectionsModel *model, QWidget *parent) :
    AbstractAddrDock(model, parent)
{
    setWindowTitle(tr("Raw"));
    connect(this, &QDockWidget::featuresChanged, this, [ = ](){
        setFeatures(QDockWidget::NoDockWidgetFeatures);
    });
}


VirtualAddrDock::VirtualAddrDock(SectionsModel *model, QWidget *parent) :
    AbstractAddrDock(model, parent)
{
    setWindowTitle(tr("Virtual"));
    connect(this, &QDockWidget::featuresChanged, this, [ = ](){
        setFeatures(QDockWidget::DockWidgetClosable);
    });
}

void RawAddrDock::updateDock()
{
    AbstractAddrDock::updateDock();
    setFeatures(QDockWidget::DockWidgetClosable);
}

void VirtualAddrDock::updateDock()
{
    AbstractAddrDock::updateDock();
    setFeatures(QDockWidget::NoDockWidgetFeatures);
}
