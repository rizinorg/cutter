#ifndef SEARCHWIDGET_H
#define SEARCHWIDGET_H

#include <memory>

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

#include "core/Cutter.h"
#include "CutterDockWidget.h"
#include "AddressableItemList.h"

class MainWindow;
class QTreeWidgetItem;
class SearchWidget;

class SearchModel : public AddressableItemModel<QAbstractListModel>
{
    Q_OBJECT

    friend SearchWidget;

private:
    QList<SearchDescription> *search;

public:
    enum Columns { OFFSET = 0, SIZE, CODE, DATA, COMMENT, COUNT };
    static const int SearchDescriptionRole = Qt::UserRole;

    SearchModel(QList<SearchDescription> *search, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    RVA address(const QModelIndex &index) const override;
};

class SearchSortFilterProxyModel : public AddressableFilterProxyModel
{
    Q_OBJECT

public:
    SearchSortFilterProxyModel(SearchModel *source_model, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

namespace Ui {
class SearchWidget;
}

class SearchWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit SearchWidget(MainWindow *main);
    ~SearchWidget();

private slots:
    void searchChanged();
    void updateSearchBoundaries();
    void refreshSearchspaces();

private:
    std::unique_ptr<Ui::SearchWidget> ui;

    SearchModel *search_model;
    SearchSortFilterProxyModel *search_proxy_model;
    QList<SearchDescription> search;

    void refreshSearch();
    void checkSearchResultEmpty();
    void enableSearch();
    void disableSearch();
    void setScrollMode();
    void updatePlaceholderText(int index);
};

#endif // SEARCHWIDGET_H
