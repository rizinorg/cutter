#ifndef SYMBOLSWIDGET_H
#define SYMBOLSWIDGET_H

#include <memory>
#include <QAbstractListModel>
#include <QSortFilterProxyModel>

#include "core/Cutter.h"
#include "CutterDockWidget.h"
#include "widgets/ListDockWidget.h"


class MainWindow;
class QTreeWidgetItem;
class SymbolsWidget;


class SymbolsModel: public AddressableItemModel<QAbstractListModel>
{
    Q_OBJECT

    friend SymbolsWidget;

private:
    QList<SymbolDescription> *symbols;

public:
    enum Column { AddressColumn = 0, TypeColumn, NameColumn, ColumnCount };
    enum Role { SymbolDescriptionRole = Qt::UserRole };

    SymbolsModel(QList<SymbolDescription> *exports, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    RVA address(const QModelIndex &index) const override;
    QString name(const QModelIndex &index) const override;
};

class SymbolsProxyModel : public AddressableFilterProxyModel
{
    Q_OBJECT

public:
    SymbolsProxyModel(SymbolsModel *sourceModel, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};


class SymbolsWidget : public ListDockWidget
{
    Q_OBJECT

public:
    explicit SymbolsWidget(MainWindow *main, QAction *action = nullptr);
    ~SymbolsWidget();

private slots:
    void refreshSymbols();

private:
    QList<SymbolDescription> symbols;
    SymbolsModel *symbolsModel;
    SymbolsProxyModel *symbolsProxyModel;
};

#endif // SYMBOLSWIDGET_H
