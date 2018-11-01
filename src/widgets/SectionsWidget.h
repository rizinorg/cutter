#ifndef SECTIONSWIDGET_H
#define SECTIONSWIDGET_H

#include <memory>

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QGraphicsScene>

#include "Cutter.h"
#include "CutterDockWidget.h"

class QTreeView;
class QAbstractItemView;
class MainWindow;
class SectionsWidget;
class SectionAddrDock;
class QuickFilterView;
class QGraphicsView;

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

private:
    QList<SectionDescription> sections;
    SectionsModel *sectionsModel;
    QTreeView *sectionsTable;
    MainWindow *main;
    QWidget *dockWidgetContents;
    QuickFilterView *quickFilterView;

    SectionAddrDock *rawAddrDock;
    SectionAddrDock *virtualAddrDock;
};

class SectionAddrDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit SectionAddrDock(QWidget *parent = nullptr);

private:
    QGraphicsView *graphicsView;
    QGraphicsScene *graphicsScene;
};

#endif // SECTIONSWIDGET_H
