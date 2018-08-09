#ifndef STRINGSWIDGET_H
#define STRINGSWIDGET_H

#include <memory>

#include "Cutter.h"
#include "CutterDockWidget.h"
#include "utils/StringsTask.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

class MainWindow;
class QTreeWidgetItem;

namespace Ui {
class StringsWidget;
}

class StringsModel: public QAbstractListModel
{
    Q_OBJECT

private:
    QList<StringDescription> *strings;

public:
    enum Columns { OFFSET = 0, STRING, TYPE, LENGTH, SIZE, COUNT };
    static const int StringDescriptionRole = Qt::UserRole;

    StringsModel(QList<StringDescription> *strings, QObject *parent = nullptr);

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
    StringsSortFilterProxyModel(StringsModel *source_model, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};


class StringsWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit StringsWidget(MainWindow *main, QAction *action = nullptr);
    ~StringsWidget();

private slots:
    void on_stringsTreeView_doubleClicked(const QModelIndex &index);

    void refreshStrings();
    void stringSearchFinished(const QList<StringDescription> &strings);

private:
    std::unique_ptr<Ui::StringsWidget> ui;

    QSharedPointer<StringsTask> task;

    StringsModel *model;
    StringsSortFilterProxyModel *proxy_model;
    QList<StringDescription> strings;
};

#endif // STRINGSWIDGET_H
