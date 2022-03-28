#ifndef SECTIONSWIDGET_H
#define SECTIONSWIDGET_H

#include <memory>
#include <map>

#include <QtWidgets/QToolButton>
#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QGraphicsScene>
#include <QLabel>
#include <QHash>

#include "core/Cutter.h"
#include "CutterDockWidget.h"
#include "widgets/ListDockWidget.h"

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

class SectionsModel : public AddressableItemModel<QAbstractListModel>
{
    Q_OBJECT

    friend SectionsWidget;

private:
    QList<SectionDescription> *sections;

public:
    enum Column {
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
    enum Role { SectionDescriptionRole = Qt::UserRole };

    SectionsModel(QList<SectionDescription> *sections, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    RVA address(const QModelIndex &index) const override;
    QString name(const QModelIndex &index) const override;
};

class SectionsProxyModel : public AddressableFilterProxyModel
{
    Q_OBJECT

public:
    SectionsProxyModel(SectionsModel *sourceModel, QObject *parent = nullptr);

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

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
    QList<SectionDescription> sections;
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

    void addTextItem(QColor color, QPoint pos, QString string);
    int getAdjustedSize(int size, int validMinSize);
    int getRectWidth();
    int getIndicatorWidth();
    int getValidMinSize();

    virtual RVA getSizeOfSection(const SectionDescription &section) = 0;
    virtual RVA getAddressOfSection(const SectionDescription &section) = 0;

private:
    void drawIndicator(QString name, float ratio);
};

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
