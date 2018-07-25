#pragma once

#include <memory>

#include "Cutter.h"
#include "CutterDockWidget.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

class MainWindow;
class QTreeWidget;

namespace Ui {
class RegisterRefsWidget;
}


class MainWindow;
class QTreeWidgetItem;


class RegisterRefModel: public QAbstractListModel
{
    Q_OBJECT

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

    void beginReloadRegisterRef();
    void endReloadRegisterRef();
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
    void showRegRefContextMenu(const QPoint &pt);

private:
    std::unique_ptr<Ui::RegisterRefsWidget> ui;

    RegisterRefModel *registerRefModel;
    RegisterRefProxyModel *registerRefProxyModel;
    QList<RegisterRefDescription> registerRefs;
    QAction *actionCopyValue;
    QAction *actionCopyRef;
    void setScrollMode();
};
