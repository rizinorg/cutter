#ifndef RELOCSWIDGET_H
#define RELOCSWIDGET_H

#include <memory>
#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

#include "CutterDockWidget.h"
#include "core/Cutter.h"
#include "CutterTreeWidget.h"

class MainWindow;
class RelocsWidget;

namespace Ui {
class RelocsWidget;
}

class RelocsModel : public QAbstractTableModel
{
    Q_OBJECT

    friend RelocsWidget;

private:
    QList<RelocDescription> *relocs;

public:
    enum Column { VAddrColumn = 0, TypeColumn, NameColumn, ColumnCount };
    enum Role { RelocDescriptionRole = Qt::UserRole, AddressRole };

    RelocsModel(QList<RelocDescription> *relocs, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
};

class RelocsProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    RelocsProxyModel(RelocsModel *sourceModel, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

class RelocsWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit RelocsWidget(MainWindow *main, QAction *action = nullptr);
    ~RelocsWidget();

private slots:
    void on_relocsTreeView_doubleClicked(const QModelIndex &index);
    void refreshRelocs();

private:
    std::unique_ptr<Ui::RelocsWidget> ui;

    RelocsModel *relocsModel;
    RelocsProxyModel *relocsProxyModel;
    QList<RelocDescription> relocs;
    CutterTreeWidget *tree;

    void setScrollMode();
};

#endif // RELOCSWIDGET_H
