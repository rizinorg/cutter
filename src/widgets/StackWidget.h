#pragma once

#include <QJsonObject>
#include <memory>
#include <QStandardItem>
#include <QTableView>

#include "core/Cutter.h"
#include "CutterDockWidget.h"
#include "menus/AddressableItemContextMenu.h"

class MainWindow;

namespace Ui {
class StackWidget;
}

class StackWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit StackWidget(MainWindow *main, QAction *action = nullptr);
    ~StackWidget();

private slots:
    void updateContents();
    void setStackGrid();
    void fontsUpdatedSlot();
    void onDoubleClicked(const QModelIndex &index);
    void customMenuRequested(QPoint pos);
    void editStack();
    void onCurrentChanged(const QModelIndex &current, const QModelIndex &previous);
    void onItemChanged(QStandardItem *item);

private:
    std::unique_ptr<Ui::StackWidget> ui;
    QTableView *viewStack = new QTableView;
    QStandardItemModel *modelStack = new QStandardItemModel(1, 3, this);
    QAction *editAction;
    QAction menuText;
    RefreshDeferrer *refreshDeferrer;
    AddressableItemContextMenu addressableItemContextMenu;
    bool updatingData = false;
};
