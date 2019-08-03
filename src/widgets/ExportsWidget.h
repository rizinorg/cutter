#ifndef EXPORTSWIDGET_H
#define EXPORTSWIDGET_H

#include <memory>

#include "core/Cutter.h"
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

class ExportsModel : public AddressableItemModel<QAbstractListModel>
{
    Q_OBJECT

    friend ExportsWidget;

private:
    QList<ExportDescription> *exports;

public:
    enum Column { OffsetColumn = 0, SizeColumn, TypeColumn, NameColumn, ColumnCount };
    enum Role { ExportDescriptionRole = Qt::UserRole };

    ExportsModel(QList<ExportDescription> *exports, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

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

class ExportsWidget : public ListDockWidget
{
    Q_OBJECT

public:
    explicit ExportsWidget(MainWindow *main, QAction *action = nullptr);
    ~ExportsWidget();

private slots:
    void refreshExports();

private:
    ExportsModel *exportsModel;
    ExportsProxyModel *exportsProxyModel;
    QList<ExportDescription> exports;
};

#endif // EXPORTSWIDGET_H
