#ifndef GLOBALSWIDGET_H
#define GLOBALSWIDGET_H

#include <memory>
#include <QAbstractListModel>
#include <QSortFilterProxyModel>

#include "core/Cutter.h"
#include "CutterDockWidget.h"
#include "widgets/ListDockWidget.h"

class MainWindow;
class QTreeWidget;
class GlobalsWidget;

namespace Ui {
class GlobalsWidget;
}

class MainWindow;
class QTreeWidgetItem;

class GlobalsModel : public AddressableItemModel<QAbstractListModel>
{
    Q_OBJECT

    friend GlobalsWidget;

private:
    QList<GlobalDescription> globals;

public:
    enum Column { AddressColumn = 0, TypeColumn, NameColumn, CommentColumn, ColumnCount };
    enum Role { GlobalDescriptionRole = Qt::UserRole };

    GlobalsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    RVA address(const QModelIndex &index) const override;
    QString name(const QModelIndex &index) const override;
};

class GlobalsProxyModel : public AddressableFilterProxyModel
{
    Q_OBJECT

public:
    GlobalsProxyModel(GlobalsModel *sourceModel, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

class GlobalsWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit GlobalsWidget(MainWindow *main);
    ~GlobalsWidget();

private slots:
    void refreshGlobals();

    void editGlobal();
    void deleteGlobal();

private:
    std::unique_ptr<Ui::GlobalsWidget> ui;

    GlobalsModel *globalsModel;
    GlobalsProxyModel *globalsProxyModel;
    CutterTreeWidget *tree;

    QAction *actionEditGlobal;
    QAction *actionDeleteGlobal;
};

#endif // GLOBALSWIDGET_H
