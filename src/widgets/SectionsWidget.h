#ifndef SECTIONSWIDGET_H
#define SECTIONSWIDGET_H

#include <memory>

#include <QAbstractListModel>
#include <QAbstractScrollArea>
#include <QSortFilterProxyModel>
#include <QGraphicsScene>

#include "Cutter.h"
#include "CutterDockWidget.h"

class QTreeView;
class QAbstractItemView;
class MainWindow;
class SectionsWidget;
class SectionScrollArea;
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
    SectionScrollArea *scrollArea;
    QWidget *dockWidgetContents;
    QuickFilterView *quickFilterView;

    QGraphicsView *graphicsView;
    QGraphicsView *graphicsView2;
    QGraphicsScene *graphicsScene;
    QGraphicsScene *graphicsScene2;
};

class SectionScrollArea : public QAbstractScrollArea
{
    Q_OBJECT

    public:
        explicit SectionScrollArea(QWidget *parent = nullptr);
signals:
    protected:
        bool viewportEvent(QEvent *event) override;
};

#endif // SECTIONSWIDGET_H
