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
class SectionAddrDock;
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

    QWidget *addrDockWidget;
    SectionAddrDock *rawAddrDock;
    SectionAddrDock *virtualAddrDock;
    QToolButton *toggleButton;

    int indicatorWidth;
    int indicatorHeight;
    int indicatorParamPosY;
    void drawIndicatorOnAddrDocks();
    void updateIndicator(SectionAddrDock *targetDock, QString name, float ratio);
    void updateToggle();
};

class SectionAddrDock : public QDockWidget
{
    Q_OBJECT

    friend SectionsWidget;

private slots:
    void updateDock();
    void addTextItem(QColor color, QPoint pos, QString string);

private:
    enum AddrType { Raw = 0, Virtual };
    int heightThreshold;
    int rectOffset;
    int rectWidth;
    QColor indicatorColor;
    explicit SectionAddrDock(SectionsModel *model, AddrType type, QWidget *parent = nullptr);
    QGraphicsScene *graphicsScene;
    QGraphicsView *graphicsView;
    SectionsProxyModel *proxyModel;
    AddrType addrType;
    QHash<QString, int> namePosYMap;
    QHash<QString, int> nameHeightMap;
};

#endif // SECTIONSWIDGET_H
