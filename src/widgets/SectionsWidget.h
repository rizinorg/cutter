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

#include "Cutter.h"
#include "CutterDockWidget.h"

class CutterTreeView;
class QAbstractItemView;
class MainWindow;
class SectionsWidget;
class AbstractAddrDock;
class AddrDockScene;
class QGraphicsSceneMouseEvent;
class RawAddrDock;
class VirtualAddrDock;
class QuickFilterView;
class QGraphicsView;
class QGraphicsRectItem;

class SectionsModel : public QAbstractListModel
{
    Q_OBJECT

    friend SectionsWidget;

private:
    QList<SectionDescription> *sections;

public:
    enum Column { NameColumn = 0, SizeColumn, AddressColumn, EndAddressColumn, EntropyColumn, ColumnCount };
    enum Role { SectionDescriptionRole = Qt::UserRole };

    SectionsModel(QList<SectionDescription> *sections, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
};

class SectionsProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    SectionsProxyModel(SectionsModel *sourceModel, QObject *parent = nullptr);

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

class SectionsWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit SectionsWidget(MainWindow *main, QAction *action = nullptr);
    ~SectionsWidget();

private slots:
    void refreshSections();
    void onSectionsDoubleClicked(const QModelIndex &index);
    void onSectionsSeekChanged(RVA addr);

private:
    QList<SectionDescription> sections;
    SectionsModel *sectionsModel;
    SectionsProxyModel *proxyModel;
    CutterTreeView *sectionsTable;
    MainWindow *main;
    QWidget *dockWidgetContents;
    QuickFilterView *quickFilterView;

    RefreshDeferrer *refreshDeferrer;

    QWidget *addrDockWidget;
    RawAddrDock *rawAddrDock;
    VirtualAddrDock *virtualAddrDock;
    QToolButton *toggleButton;

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
    int indicatorWidth;
    int indicatorHeight;
    int indicatorParamPosY;
    float heightThreshold;
    float heightDivisor;
    int rectOffset;
    int rectWidth;
    QColor indicatorColor;
    QColor textColor;
    AddrDockScene *addrDockScene;
    QGraphicsView *graphicsView;
    SectionsProxyModel *proxyModel;

    void addTextItem(QColor color, QPoint pos, QString string);
    int getAdjustedSize(int size, int validMinSize);

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
    QHash<QString, int> nameAddrSizeMap;
    QHash<QString, RVA> seekAddrMap;
    QHash<QString, int> seekAddrSizeMap;
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
    ~RawAddrDock();

    void updateDock() override;
    int getValidMinSize();
};

class VirtualAddrDock : public AbstractAddrDock
{
    Q_OBJECT

public:
    explicit VirtualAddrDock(SectionsModel *model, QWidget *parent = nullptr);
    ~VirtualAddrDock();

    void updateDock() override;
    int getValidMinSize();
};

#endif // SECTIONSWIDGET_H
