#ifndef HEADERSWIDGET_H
#define HEADERSWIDGET_H

#include "CutterDescriptions.h"
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

/**
 * @brief Source model for @ref HeadersWidget
 */
class HeadersModel : public AddressableItemModel<QAbstractListModel>
{
    Q_OBJECT

    friend HeadersWidget;

private:
    QList<HeaderDescription> headers;

public:
    enum Column : ut8 { OffsetColumn = 0, NameColumn, ValueColumn, CommentColumn, ColumnCount };
    enum Role : ut16 { HeaderDescriptionRole = Qt::UserRole };

    HeadersModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    RVA address(const QModelIndex &index) const override;
    QString name(const QModelIndex &index) const override;
};

/**
 * @brief Proxy model for @ref HeadersWidget
 */
class HeadersProxyModel : public AddressableFilterProxyModel
{
    Q_OBJECT

public:
    HeadersProxyModel(HeadersModel *sourceModel, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

/**
 * @brief A dock widget that displays binary header information (e.g., ELF or PE fields) in a
 * searchable list, allowing users to inspect offsets, names, and values
 */
class HeadersWidget : public ListDockWidget
{
    Q_OBJECT

public:
    explicit HeadersWidget(MainWindow *main);
    ~HeadersWidget();

private slots:
    void refreshHeaders();

private:
    HeadersModel *headersModel;
    HeadersProxyModel *headersProxyModel;
};

#endif // HEADERSWIDGET_H
