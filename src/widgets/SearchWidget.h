#ifndef SEARCHWIDGET_H
#define SEARCHWIDGET_H

#include <memory>

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

#include "Cutter.h"
#include "CutterDockWidget.h"

class MainWindow;
class QTreeWidgetItem;


class SearchModel: public QAbstractListModel
{
    Q_OBJECT

private:
    QList<SearchDescription> *search;

public:
    enum Columns { OFFSET = 0, SIZE, CODE, DATA, COUNT };
    static const int SearchDescriptionRole = Qt::UserRole;

    SearchModel(QList<SearchDescription> *search, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    void beginReloadSearch();
    void endReloadSearch();
};



class SearchSortFilterProxyModel : public QSortFilterProxyModel
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
    explicit SearchWidget(MainWindow *main, QAction *action = nullptr);
    ~SearchWidget();

private slots:
    void on_searchTreeView_doubleClicked(const QModelIndex &index);

    void searchChanged();
    void refreshSearchspaces();

private:
    std::unique_ptr<Ui::SearchWidget> ui;

    SearchModel *search_model;
    SearchSortFilterProxyModel *search_proxy_model;
    QList<SearchDescription> search;

    void refreshSearch();
    void setScrollMode();
    void updatePlaceholderText(int index);
};

#endif // SEARCHWIDGET_H
