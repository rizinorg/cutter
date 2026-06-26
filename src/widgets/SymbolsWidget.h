#ifndef SYMBOLSWIDGET_H
#define SYMBOLSWIDGET_H

#include "CutterDescriptions.h"
#include "CutterDockWidget.h"
#include "widgets/ListDockWidget.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

class MainWindow;
class QTreeWidgetItem;
class SymbolsWidget;

/**
 * @brief Source model for @ref SymbolsWidget
 */
class SymbolsModel : public AddressableItemModel<QAbstractListModel>
{
    Q_OBJECT

    friend SymbolsWidget;

private:
    QList<SymbolDescription> symbols;

public:
    enum Column : ut8 { AddressColumn = 0, TypeColumn, NameColumn, CommentColumn, ColumnCount };
    enum Role : ut16 { SymbolDescriptionRole = Qt::UserRole };

    SymbolsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    RVA address(const QModelIndex &index) const override;
    QString name(const QModelIndex &index) const override;
};

/**
 * @brief Proxy model for @ref SymbolsWidget
 */
class SymbolsProxyModel : public AddressableFilterProxyModel
{
    Q_OBJECT

public:
    SymbolsProxyModel(SymbolsModel *sourceModel, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

/**
 * @brief Widget listing all symbols in binary
 */
class SymbolsWidget : public ListDockWidget
{
    Q_OBJECT

public:
    explicit SymbolsWidget(MainWindow *main);
    ~SymbolsWidget();

private slots:
    void refreshSymbols();

private:
    SymbolsModel *symbolsModel;
    SymbolsProxyModel *symbolsProxyModel;
};

#endif // SYMBOLSWIDGET_H
