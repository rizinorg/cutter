#ifndef SECTIONSWIDGET_H
#define SECTIONSWIDGET_H

#include "CutterDescriptions.h"
#include "CutterDockWidget.h"
#include "widgets/ListDockWidget.h"

#include <QAbstractListModel>
#include <QGraphicsScene>
#include <QHash>
#include <QLabel>
#include <QSortFilterProxyModel>
#include <QtWidgets/QToolButton>

class QAbstractItemView;
class SectionsWidget;
class AbstractAddrDock;
class AddrDockScene;
class QGraphicsSceneMouseEvent;
class RawAddrDock;
class VirtualAddrDock;
class QuickFilterView;
class QGraphicsView;
class QGraphicsRectItem;

/**
 * @brief Source model for @ref SectionsWidget
 */
class SectionsModel : public AddressableItemModel<QAbstractListModel>
{
    Q_OBJECT

    friend SectionsWidget;

private:
    QList<SectionDescription> sections;

public:
    enum Column : ut8 {
        NameColumn = 0,
        SizeColumn,
        AddressColumn,
        EndAddressColumn,
        VirtualSizeColumn,
        PermissionsColumn,
        EntropyColumn,
        CommentColumn,
        ColumnCount
    };
    enum Role : ut16 { SectionDescriptionRole = Qt::UserRole };

    SectionsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    RVA address(const QModelIndex &index) const override;
    QString name(const QModelIndex &index) const override;
};

/**
 * @brief Proxy model for @ref SectionsWidget
 */
class SectionsProxyModel : public AddressableFilterProxyModel
{
    Q_OBJECT

public:
    SectionsProxyModel(SectionsModel *sourceModel, QObject *parent = nullptr);

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

/**
 * @brief Widget showing list of all sections in a binary
 */
class SectionsWidget : public ListDockWidget
{
    Q_OBJECT

public:
    explicit SectionsWidget(MainWindow *main);
    ~SectionsWidget();

private slots:
    void refreshSections();
    void refreshDocks();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    SectionsModel *sectionsModel;
    SectionsProxyModel *proxyModel;

    QWidget *addrDockWidget;
    RawAddrDock *rawAddrDock;
    VirtualAddrDock *virtualAddrDock;
    QToolButton *toggleButton;

    /**
     * RefreshDeferrer for loading the section data
     */
    RefreshDeferrer *sectionsRefreshDeferrer;

    /**
     * RefreshDeferrer for updating the visualization docks
     */
    RefreshDeferrer *dockRefreshDeferrer;

    void initSectionsTable();
    void initQuickFilter();
    void initConnects();
    void initAddrMapDocks();
    void drawIndicatorOnAddrDocks();
    void updateToggle();
};

/**
 * @brief A base class for visual address maps that uses a QGraphicsView to render binary segments.
 *
 * It defines the core geometry (offsets, widths, and scaling) and requires subclasses to implement
 * specific logic for retrieving addresses and sizes.
 */
class AbstractAddrDock : public QDockWidget
{
    Q_OBJECT

    friend SectionsWidget;

public:
    explicit AbstractAddrDock(SectionsModel *model, QWidget *parent = nullptr);
    ~AbstractAddrDock();

    virtual void updateDock();

protected:
    int indicatorHeight;
    int indicatorParamPosY;
    float heightThreshold;
    float heightDivisor;
    int rectOffset;
    int rectWidthMin;
    int rectWidthMax;
    QColor indicatorColor;
    QColor textColor;
    AddrDockScene *addrDockScene;
    QGraphicsView *graphicsView;
    SectionsProxyModel *proxyModel;

    void addTextItem(QColor color, QPoint pos, const QString &string);
    int getAdjustedSize(int size, int validMinSize) const;
    int getRectWidth();
    int getIndicatorWidth();
    int getValidMinSize();

    virtual RVA getSizeOfSection(const SectionDescription &section) = 0;
    virtual RVA getAddressOfSection(const SectionDescription &section) = 0;

private:
    void drawIndicator(const QString &name, float ratio);
};

/**
 * @brief Graphics scene that translates mouse coordinates into memory addresses for navigation and
 * "seek" interactions.
 */
class AddrDockScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit AddrDockScene(QWidget *parent = nullptr);
    ~AddrDockScene();

    bool disableCenterOn;

    QHash<QString, RVA> nameAddrMap;
    QHash<QString, RVA> nameAddrSizeMap;
    QHash<QString, RVA> seekAddrMap;
    QHash<QString, RVA> seekAddrSizeMap;
    QHash<QString, int> namePosYMap;
    QHash<QString, int> nameHeightMap;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;

private:
    RVA getAddrFromPos(int posY, bool seek);
};

/**
 * @brief Visualizes the binary's physical layout using paddr
 */
class RawAddrDock : public AbstractAddrDock
{
    Q_OBJECT

public:
    explicit RawAddrDock(SectionsModel *model, QWidget *parent = nullptr);
    ~RawAddrDock() = default;

    void updateDock() override;

protected:
    RVA getSizeOfSection(const SectionDescription &section) override { return section.size; };
    RVA getAddressOfSection(const SectionDescription &section) override { return section.paddr; };
};

/**
 * @brief Visualizes the binary's memory layout using virtual addresses (vaddr)
 */
class VirtualAddrDock : public AbstractAddrDock
{
    Q_OBJECT

public:
    explicit VirtualAddrDock(SectionsModel *model, QWidget *parent = nullptr);
    ~VirtualAddrDock() = default;

    void updateDock() override;

protected:
    RVA getSizeOfSection(const SectionDescription &section) override { return section.vsize; };
    RVA getAddressOfSection(const SectionDescription &section) override { return section.vaddr; };
};

#endif // SECTIONSWIDGET_H
