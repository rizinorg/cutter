#ifndef HEADERSWIDGET_H
#define HEADERSWIDGET_H

#include <memory>

#include "core/Cutter.h"
#include "ListDockWidget.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

class MainWindow;
class QTreeWidget;

namespace Ui {
class HeadersWidget;
}


class MainWindow;
class QTreeWidgetItem;
class HeadersWidget;


class HeadersModel: public AddressableItemModel<QAbstractListModel>
{
    Q_OBJECT

    friend HeadersWidget;

private:
    QList<HeaderDescription> *headers;

public:
    enum Column { OffsetColumn = 0, NameColumn, ValueColumn, ColumnCount };
    enum Role { HeaderDescriptionRole = Qt::UserRole };

    HeadersModel(QList<HeaderDescription> *headers, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    RVA address(const QModelIndex &index) const override;
    QString name(const QModelIndex &index) const override;
};



class HeadersProxyModel : public AddressableFilterProxyModel
{
    Q_OBJECT

public:
    HeadersProxyModel(HeadersModel *sourceModel, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};



class HeadersWidget : public ListDockWidget
{
    Q_OBJECT

public:
    explicit HeadersWidget(MainWindow *main, QAction *action = nullptr);
    ~HeadersWidget();

private slots:
    void refreshHeaders();

private:
    HeadersModel *headersModel;
    HeadersProxyModel *headersProxyModel;
    QList<HeaderDescription> headers;
};


#endif // HEADERSWIDGET_H
