#ifndef ZIGNATURESWIDGET_H
#define ZIGNATURESWIDGET_H

#include <memory>

#include "Cutter.h"
#include "CutterDockWidget.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

class MainWindow;
class QTreeWidget;
class QTreeWidgetItem;

namespace Ui
{
    class ZignaturesWidget;
}

class ZignaturesModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Column { OffsetColumn = 0, NameColumn, ValueColumn, ColumnCount };
    enum Role { ZignatureDescriptionRole = Qt::UserRole };

    ZignaturesModel(QList<ZignatureDescription> *zignatures, QObject *parent = 0);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    void beginReloadZignatures();
    void endReloadZignatures();

private:
    QList<ZignatureDescription> *zignatures;
};



class ZignaturesProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    ZignaturesProxyModel(ZignaturesModel *sourceModel, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};



class ZignaturesWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit ZignaturesWidget(MainWindow *main, QAction *action = nullptr);
    ~ZignaturesWidget();

private slots:
    void on_zignaturesTreeView_doubleClicked(const QModelIndex &index);

    void refreshZignatures();

private:
    std::unique_ptr<Ui::ZignaturesWidget> ui;

    ZignaturesModel *zignaturesModel;
    ZignaturesProxyModel *zignaturesProxyModel;
    QList<ZignatureDescription> zignatures;

    void setScrollMode();
};

#endif // ZIGNATURESWIDGET_H
