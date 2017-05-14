#ifndef FLAGSWIDGET_H
#define FLAGSWIDGET_H

#include "qrcore.h"
#include "dockwidget.h"
#include <QAbstractItemModel>
#include <QSortFilterProxyModel>


class MainWindow;
class QTreeWidgetItem;


class FlagsModel: public QAbstractListModel
{
    Q_OBJECT

private:
    QList<FlagDescription> *flags;

public:
    enum Columns { OFFSET = 0, SIZE, NAME, COUNT };
    static const int FlagDescriptionRole = Qt::UserRole;

    FlagsModel(QList<FlagDescription> *flags, QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    void beginReloadFlags();
    void endReloadFlags();
};



class FlagsSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    FlagsSortFilterProxyModel(FlagsModel *source_model, QObject *parent = 0);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};



namespace Ui
{
    class FlagsWidget;
}

class FlagsWidget : public DockWidget
{
    Q_OBJECT

public:
    explicit FlagsWidget(MainWindow *main, QWidget *parent = 0);
    ~FlagsWidget();

    void setup() override;
    void refresh() override;

private slots:
    void on_flagsTreeView_doubleClicked(const QModelIndex &index);
    void on_flagspaceCombo_currentTextChanged(const QString &arg1);

private:
    Ui::FlagsWidget *ui;
    MainWindow      *main;

    FlagsModel *flags_model;
    FlagsSortFilterProxyModel *flags_proxy_model;
    QList<FlagDescription> flags;

    void refreshFlags();
    void refreshFlagspaces();
    void setScrollMode();
};

#endif // FLAGSWIDGET_H
