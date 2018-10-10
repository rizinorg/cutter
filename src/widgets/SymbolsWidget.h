#ifndef SYMBOLSWIDGET_H
#define SYMBOLSWIDGET_H

#include <memory>
#include <QAbstractListModel>
#include <QSortFilterProxyModel>

#include "Cutter.h"
#include "CutterDockWidget.h"
#include "CutterTreeWidget.h"

class MainWindow;
class QTreeWidgetItem;
class SymbolsWidget;

namespace Ui {
class SymbolsWidget;
}

class SymbolsModel: public QAbstractListModel
{
    Q_OBJECT

    friend SymbolsWidget;

private:
    QList<SymbolDescription> *symbols;

public:
    enum Column { AddressColumn = 0, TypeColumn, NameColumn, ColumnCount };
    enum Role { SymbolDescriptionRole = Qt::UserRole };

    SymbolsModel(QList<SymbolDescription> *exports, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
};

class SymbolsProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    SymbolsProxyModel(SymbolsModel *sourceModel, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};


class SymbolsWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit SymbolsWidget(MainWindow *main, QAction *action = nullptr);
    ~SymbolsWidget();

private slots:
    void on_symbolsTreeView_doubleClicked(const QModelIndex &index);

    void refreshSymbols();

private:
    std::unique_ptr<Ui::SymbolsWidget> ui;

    QList<SymbolDescription> symbols;
    SymbolsModel *symbolsModel;
    SymbolsProxyModel *symbolsProxyModel;
    CutterTreeWidget *tree;

    void setScrollMode();
};

#endif // SYMBOLSWIDGET_H
