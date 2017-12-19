#ifndef STRINGSWIDGET_H
#define STRINGSWIDGET_H

#include <memory>

#include "cutter.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QDockWidget>

class MainWindow;
class QTreeWidgetItem;

namespace Ui
{
    class StringsWidget;
}



class StringsModel: public QAbstractListModel
{
Q_OBJECT

private:
    QList<StringDescription> *strings;

public:
    enum Columns { OFFSET = 0, STRING, COUNT };
    static const int StringDescriptionRole = Qt::UserRole;

    StringsModel(QList<StringDescription> *strings, QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    void beginReload();
    void endReload();
};



class StringsSortFilterProxyModel : public QSortFilterProxyModel
{
Q_OBJECT

public:
    StringsSortFilterProxyModel(StringsModel *source_model, QObject *parent = 0);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};


class StringsWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit StringsWidget(QWidget *parent = nullptr);
    ~StringsWidget();

private slots:
    void on_stringsTreeView_doubleClicked(const QModelIndex &index);

    void refreshStrings();

private:
    std::unique_ptr<Ui::StringsWidget> ui;

    StringsModel *model;
    StringsSortFilterProxyModel *proxy_model;
    QList<StringDescription> strings;
};

#endif // STRINGSWIDGET_H
