#pragma once

#include <memory>

#include "core/Cutter.h"
#include "CutterDockWidget.h"
#include "CutterTreeWidget.h"
#include "menus/AddressableItemContextMenu.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

class MainWindow;
class QTreeWidget;
class RegisterRefsWidget;

namespace Ui {
class RegisterRefsWidget;
}


class MainWindow;
class QTreeWidgetItem;

struct RegisterRefDescription {
    QString reg;
    QString value;
    RefDescription refDesc;
};
Q_DECLARE_METATYPE(RegisterRefDescription)

class RegisterRefModel: public QAbstractListModel
{
    Q_OBJECT

    friend RegisterRefsWidget;

private:
    QList<RegisterRefDescription> *registerRefs;

public:
    enum Column { RegColumn = 0, ValueColumn, RefColumn, ColumnCount };
    enum Role { RegisterRefDescriptionRole = Qt::UserRole };

    RegisterRefModel(QList<RegisterRefDescription> *registerRefs, QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
};

class RegisterRefProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    RegisterRefProxyModel(RegisterRefModel *sourceModel, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};


class RegisterRefsWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit RegisterRefsWidget(MainWindow *main, QAction *action = nullptr);
    ~RegisterRefsWidget();

private slots:
    void on_registerRefTreeView_doubleClicked(const QModelIndex &index);
    void refreshRegisterRef();
    void copyClip(int column);
    void customMenuRequested(QPoint pos);
    void onCurrentChanged(const QModelIndex &current, const QModelIndex &previous);

private:
    std::unique_ptr<Ui::RegisterRefsWidget> ui;

    RegisterRefModel *registerRefModel;
    RegisterRefProxyModel *registerRefProxyModel;
    QList<RegisterRefDescription> registerRefs;
    CutterTreeWidget *tree;
    void setScrollMode();

    RefreshDeferrer *refreshDeferrer;

    QAction *actionCopyValue;
    QAction *actionCopyRef;
    AddressableItemContextMenu addressableItemContextMenu;
};
