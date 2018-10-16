#ifndef FLAGSWIDGET_H
#define FLAGSWIDGET_H

#include <memory>

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

#include "Cutter.h"
#include "CutterDockWidget.h"
#include "CutterTreeWidget.h"

class MainWindow;
class QTreeWidgetItem;
class FlagsWidget;


class FlagsModel: public QAbstractListModel
{
    Q_OBJECT

    friend FlagsWidget;

private:
    QList<FlagDescription> *flags;

public:
    enum Columns { OFFSET = 0, SIZE, NAME, COUNT };
    static const int FlagDescriptionRole = Qt::UserRole;

    FlagsModel(QList<FlagDescription> *flags, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
};



class FlagsSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    FlagsSortFilterProxyModel(FlagsModel *source_model, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};



namespace Ui {
class FlagsWidget;
}

class FlagsWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit FlagsWidget(MainWindow *main, QAction *action = nullptr);
    ~FlagsWidget();

private slots:
    void on_flagsTreeView_doubleClicked(const QModelIndex &index);
    void on_flagspaceCombo_currentTextChanged(const QString &arg1);

    void on_actionRename_triggered();
    void on_actionDelete_triggered();

    void showContextMenu(const QPoint &pt);

    void flagsChanged();
    void refreshFlagspaces();

private:
    std::unique_ptr<Ui::FlagsWidget> ui;
    MainWindow *main;

    FlagsModel *flags_model;
    FlagsSortFilterProxyModel *flags_proxy_model;
    QList<FlagDescription> flags;
    CutterTreeWidget *tree;

    void refreshFlags();
    void setScrollMode();
};

#endif // FLAGSWIDGET_H
