#ifndef EXPORTSWIDGET_H
#define EXPORTSWIDGET_H

#include <memory>

#include "Cutter.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QDockWidget>

class MainWindow;
class QTreeWidget;

namespace Ui
{
    class ExportsWidget;
}


class MainWindow;
class QTreeWidgetItem;


class ExportsModel: public QAbstractListModel
{
    Q_OBJECT

private:
    QList<ExportDescription> *exports;

public:
    enum Columns { OFFSET = 0, SIZE, TYPE, NAME, COUNT };
    static const int ExportDescriptionRole = Qt::UserRole;

    ExportsModel(QList<ExportDescription> *exports, QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    void beginReloadExports();
    void endReloadExports();
};



class ExportsSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    ExportsSortFilterProxyModel(ExportsModel *source_model, QObject *parent = 0);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};



class ExportsWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit ExportsWidget(MainWindow *main, QWidget *parent = 0);
    ~ExportsWidget();

private slots:
    void on_exportsTreeView_doubleClicked(const QModelIndex &index);

    void refreshExports();

private:
    std::unique_ptr<Ui::ExportsWidget> ui;
    MainWindow      *main;

    ExportsModel *exports_model;
    ExportsSortFilterProxyModel *exports_proxy_model;
    QList<ExportDescription> exports;

    void setScrollMode();
};


#endif // EXPORTSWIDGET_H
