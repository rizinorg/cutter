#ifndef TYPESWIDGET_H
#define TYPESWIDGET_H

#include <memory>

#include "Cutter.h"
#include "CutterDockWidget.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

class MainWindow;
class QTreeWidget;

namespace Ui {
class TypesWidget;
}


class MainWindow;
class QTreeWidgetItem;


class TypesModel: public QAbstractListModel
{
    Q_OBJECT

private:
    QList<TypeDescription> *types;

public:
    enum Columns { TYPE = 0, SIZE, FORMAT, COUNT };
    static const int TypeDescriptionRole = Qt::UserRole;

    TypesModel(QList<TypeDescription> *types, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    void beginReloadTypes();
    void endReloadTypes();
};



class TypesSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    TypesSortFilterProxyModel(TypesModel *source_model, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};



class TypesWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit TypesWidget(MainWindow *main, QAction *action = nullptr);
    ~TypesWidget();

private slots:
    void on_typesTreeView_doubleClicked(const QModelIndex &index);

    void refreshTypes();

private:
    std::unique_ptr<Ui::TypesWidget> ui;

    TypesModel *types_model;
    TypesSortFilterProxyModel *types_proxy_model;
    QList<TypeDescription> types;

    void setScrollMode();
};


#endif // TYPESWIDGET_H
