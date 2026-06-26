#ifndef FLAGSWIDGET_H
#define FLAGSWIDGET_H

#include "AddressableItemList.h"
#include "AddressableItemModel.h"
#include "CutterDescriptions.h"
#include "CutterDockWidget.h"

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>

#include <memory>

class MainWindow;
class QTreeWidgetItem;
class FlagsWidget;

/**
 * @brief Source model for @ref FlagsWidget
 */
class FlagsModel : public AddressableItemModel<QAbstractListModel>
{
    Q_OBJECT
    friend FlagsWidget;

private:
    QList<FlagDescription> flags;

public:
    enum Columns : ut8 { OFFSET = 0, SIZE, NAME, REALNAME, COMMENT, COUNT };
    static const int flagDescriptionRole = Qt::UserRole;

    FlagsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    RVA address(const QModelIndex &index) const override;
    QString name(const QModelIndex &index) const override;

    const FlagDescription *description(QModelIndex index) const;
};

/**
 * @brief Sort and filter proxy model for @ref FlagsWidget
 */
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

/**
 * @brief Widget for listing and modifying all flags
 */
class FlagsWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit FlagsWidget(MainWindow *main);
    ~FlagsWidget();

private slots:
    void onFlagspaceComboCurrentTextChanged(const QString &arg1);

    void onActionRenameTriggered();
    void onActionDeleteTriggered();

    void flagsChanged();
    void refreshFlagspaces();

private:
    std::unique_ptr<Ui::FlagsWidget> ui;
    MainWindow *main;

    bool disableFlagRefresh = false;
    FlagsModel *flagsModel;
    FlagsSortFilterProxyModel *flagsProxyModel;

    void refreshFlags();
    void setScrollMode();
};

#endif // FLAGSWIDGET_H
