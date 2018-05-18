#ifndef HEADERSWIDGET_H
#define HEADERSWIDGET_H

#include <memory>

#include "Cutter.h"
#include "CutterDockWidget.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

class MainWindow;
class QTreeWidget;

namespace Ui
{
    class HeadersWidget;
}


class MainWindow;
class QTreeWidgetItem;


class HeadersModel: public QAbstractListModel
{
    Q_OBJECT

private:
    QList<HeaderDescription> *headers;

public:
    enum Column { OffsetColumn = 0, NameColumn, ValueColumn, ColumnCount };
    enum Role { HeaderDescriptionRole = Qt::UserRole };

    HeadersModel(QList<HeaderDescription> *headers, QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    void beginReloadHeaders();
    void endReloadHeaders();
};



class HeadersProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    HeadersProxyModel(HeadersModel *sourceModel, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};



class HeadersWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit HeadersWidget(MainWindow *main, QAction *action = nullptr);
    ~HeadersWidget();

private slots:
    void on_headersTreeView_doubleClicked(const QModelIndex &index);

    void refreshHeaders();

private:
    std::unique_ptr<Ui::HeadersWidget> ui;

    HeadersModel *headersModel;
    HeadersProxyModel *headersProxyModel;
    QList<HeaderDescription> headers;

    void setScrollMode();
};


#endif // HEADERSWIDGET_H
