#ifndef SEARCHWIDGET_H
#define SEARCHWIDGET_H

#include "AddressableItemList.h"
#include "CutterDockWidget.h"

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

#include <memory>

class MainWindow;
class QTreeWidgetItem;
class SearchWidget;

/**
 * @brief Souce model for @ref SearchWidget
 */
class SearchModel : public AddressableItemModel<QAbstractListModel>
{
    Q_OBJECT

    friend SearchWidget;

private:
    QList<SearchDescription> search;

public:
    enum Columns : ut8 { OFFSET = 0, SIZE, CODE, DATA, COMMENT, COUNT };
    static const int searchDescriptionRole = Qt::UserRole;

    SearchModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    RVA address(const QModelIndex &index) const override;
};

/**
 * @brief Sort and Filter proxy model for @ref SearchWidget
 */
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

/**
 * @brief Widget for searching
 */
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
    void runSearch();
    void updateColors();

private:
    std::unique_ptr<Ui::SearchWidget> ui;

    SearchModel *searchModel;
    SearchSortFilterProxyModel *searchProxyModel;

    void refreshSearch();
    void checkSearchResultEmpty();
    void enableSearch();
    void disableSearch();
    void setScrollMode();
    void updatePlaceholderText(int index);
};

#endif // SEARCHWIDGET_H
