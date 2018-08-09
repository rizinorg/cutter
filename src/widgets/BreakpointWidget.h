#pragma once

#include <memory>

#include "Cutter.h"
#include "CutterDockWidget.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

class MainWindow;
class QTreeWidget;

namespace Ui
{
    class BreakpointWidget;
}


class MainWindow;
class QTreeWidgetItem;


class BreakpointModel: public QAbstractListModel
{
    Q_OBJECT

private:
    QList<BreakpointDescription> *breakpoints;

public:
    enum Column { AddrColumn = 0, PermColumn, HwColumn, TraceColumn, EnabledColumn, ColumnCount };
    enum Role { BreakpointDescriptionRole = Qt::UserRole };

    BreakpointModel(QList<BreakpointDescription> *breakpoints, QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    void beginReloadBreakpoint();
    void endReloadBreakpoint();
};



class BreakpointProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    BreakpointProxyModel(BreakpointModel *sourceModel, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};



class BreakpointWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit BreakpointWidget(MainWindow *main, QAction *action = nullptr);
    ~BreakpointWidget();

private slots:
    void on_breakpointTreeView_doubleClicked(const QModelIndex &index);
    void showBreakpointContextMenu(const QPoint &pt);
    void delBreakpoint();
    void toggleBreakpoint();
    void addBreakpointDialog();
    void refreshBreakpoint();

private:
    std::unique_ptr<Ui::BreakpointWidget> ui;

    BreakpointModel *breakpointModel;
    BreakpointProxyModel *breakpointProxyModel;
    QList<BreakpointDescription> breakpoints;
    QAction *actionDelBreakpoint = nullptr;
    QAction *actionToggleBreakpoint = nullptr;

    void setScrollMode();
};
