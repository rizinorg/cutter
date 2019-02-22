#ifndef SEGMENTSWIDGET_H
#define SEGMENTSWIDGET_H

#include <memory>

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

#include "core/Cutter.h"
#include "CutterTreeView.h"
#include "CutterDockWidget.h"

class CutterTreeView;
class QAbstractItemView;
class MainWindow;
class SegmentsWidget;
class QuickFilterView;

class SegmentsModel : public QAbstractListModel
{
    Q_OBJECT

    friend SegmentsWidget;

private:
    QList<SegmentDescription> *segments;

public:
    enum Column { NameColumn = 0, SizeColumn, AddressColumn, EndAddressColumn, PermColumn, ColumnCount };
    enum Role { SegmentDescriptionRole = Qt::UserRole };

    SegmentsModel(QList<SegmentDescription> *segments, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int segment, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
};

class SegmentsProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    SegmentsProxyModel(SegmentsModel *sourceModel, QObject *parent = nullptr);

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

class SegmentsWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit SegmentsWidget(MainWindow *main, QAction *action = nullptr);
    ~SegmentsWidget();

private slots:
    void refreshSegments();
    void onSegmentsDoubleClicked(const QModelIndex &index);

private:
    QList<SegmentDescription> segments;
    SegmentsModel *segmentsModel;
    CutterTreeView *segmentsTable;
    MainWindow *main;
    QWidget *dockWidgetContents;
    QuickFilterView *quickFilterView;
};

#endif // SEGMENTSWIDGET_H
