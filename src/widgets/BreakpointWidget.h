#pragma once

#include <memory>

#include "core/Cutter.h"
#include "CutterDockWidget.h"
#include "AddressableItemModel.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

class MainWindow;
class QTreeWidget;

namespace Ui {
class BreakpointWidget;
}


class MainWindow;
class QTreeWidgetItem;
class BreakpointWidget;

class BreakpointModel: public AddressableItemModel<QAbstractListModel>
{
    Q_OBJECT

    friend BreakpointWidget;

private:
    QList<BreakpointDescription> breakpoints;

public:
    enum Column { AddrColumn = 0, NameColumn, TypeColumn, TraceColumn, EnabledColumn, ColumnCount };
    enum Role { BreakpointDescriptionRole = Qt::UserRole };

    BreakpointModel(QObject *parent = nullptr);

    void refresh();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;


    RVA address(const QModelIndex &index) const override;
};



class BreakpointProxyModel : public AddressableFilterProxyModel
{
    Q_OBJECT

public:
    BreakpointProxyModel(BreakpointModel *sourceModel, QObject *parent = nullptr);

};



class BreakpointWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit BreakpointWidget(MainWindow *main);
    ~BreakpointWidget();

private slots:
    void delBreakpoint();
    void toggleBreakpoint();
    void editBreakpoint();
    void addBreakpointDialog();
    void refreshBreakpoint();

private:
    std::unique_ptr<Ui::BreakpointWidget> ui;

    BreakpointModel *breakpointModel;
    BreakpointProxyModel *breakpointProxyModel;
    QList<BreakpointDescription> breakpoints;
    QAction *actionDelBreakpoint = nullptr;
    QAction *actionToggleBreakpoint = nullptr;
    QAction *actionEditBreakpoint = nullptr;

    void setScrollMode();
    QVector<RVA> getSelectedAddresses() const;

    RefreshDeferrer *refreshDeferrer;
    bool editing = false;
};
