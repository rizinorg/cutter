#ifndef FLAGSWIDGET_H
#define FLAGSWIDGET_H

#include <memory>

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>

#include "core/Cutter.h"
#include "CutterDockWidget.h"
#include "CutterTreeWidget.h"
#include "AddressableItemList.h"
#include "AddressableItemModel.h"

class MainWindow;
class QTreeWidgetItem;
class FlagsWidget;


class FlagsModel: public AddressableItemModel<QAbstractListModel>
{
    friend FlagsWidget;

private:
    QList<FlagDescription> *flags;

public:
    enum Columns { OFFSET = 0, SIZE, NAME, REALNAME, COUNT };
    static const int FlagDescriptionRole = Qt::UserRole;

    FlagsModel(QList<FlagDescription> *flags, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    RVA address(const QModelIndex &index) const override;
    QString name(const QModelIndex &index) const override;

    const FlagDescription *description(QModelIndex index) const;
};



class FlagsSortFilterProxyModel : public AddressableFilterProxyModel
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
    void on_flagspaceCombo_currentTextChanged(const QString &arg1);

    void on_actionRename_triggered();
    void on_actionDelete_triggered();

    void flagsChanged();
    void refreshFlagspaces();

private:
    std::unique_ptr<Ui::FlagsWidget> ui;
    MainWindow *main;

    bool disableFlagRefresh = false;
    FlagsModel *flags_model;
    FlagsSortFilterProxyModel *flags_proxy_model;
    QList<FlagDescription> flags;
    CutterTreeWidget *tree;

    void refreshFlags();
    void setScrollMode();
};

#endif // FLAGSWIDGET_H
