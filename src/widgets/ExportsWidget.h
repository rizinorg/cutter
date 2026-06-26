#ifndef EXPORTSWIDGET_H
#define EXPORTSWIDGET_H

#include "CutterDescriptions.h"
#include "CutterDockWidget.h"
#include "widgets/ListDockWidget.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

class MainWindow;
class QTreeWidget;
class ExportsWidget;

namespace Ui {
class ExportsWidget;
}

/**
 * @brief Source model for @ref ExportsWidget
 */
class ExportsModel : public AddressableItemModel<QAbstractListModel>
{
    Q_OBJECT

    friend ExportsWidget;

private:
    QList<ExportDescription> exports;

public:
    enum Column : ut8 {
        OffsetColumn = 0,
        SizeColumn,
        TypeColumn,
        NameColumn,
        CommentColumn,
        ColumnCount
    };
    enum Role : ut16 { ExportDescriptionRole = Qt::UserRole };

    ExportsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    RVA address(const QModelIndex &index) const override;
    QString name(const QModelIndex &index) const override;
};

class ExportsProxyModel : public AddressableFilterProxyModel
{
    Q_OBJECT

public:
    ExportsProxyModel(ExportsModel *source_model, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

/**
 * @brief Widget listing all exports inside the binary
 */
class ExportsWidget : public ListDockWidget
{
    Q_OBJECT

public:
    explicit ExportsWidget(MainWindow *main);
    ~ExportsWidget();

private slots:
    void refreshExports();

private:
    ExportsModel *exportsModel;
    ExportsProxyModel *exportsProxyModel;
};

#endif // EXPORTSWIDGET_H
