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

class StackModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    struct Item {
        RVA offset;
        QString value;
        RefDescription refDesc;
    };

    enum Column { OffsetColumn = 0, ValueColumn, DescriptionColumn, ColumnCount};
    enum Role { StackDescriptionRole = Qt::UserRole };

    StackModel(QObject *parent = nullptr);

    void reload();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

private:
    QVector<Item> values;

};
Q_DECLARE_METATYPE(StackModel::Item)

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
private:
    std::unique_ptr<Ui::StackWidget> ui;
    QTableView *viewStack = new QTableView(this);
    StackModel *modelStack = new StackModel(this);
    QAction *editAction;
    QAction menuText;
    RefreshDeferrer *refreshDeferrer;
    AddressableItemContextMenu addressableItemContextMenu;

};
