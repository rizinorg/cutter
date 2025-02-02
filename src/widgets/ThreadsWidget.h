#pragma once

#include <QSortFilterProxyModel>

#include "core/Cutter.h"
#include "AddressableItemContextMenu.h"
#include "CutterDescriptions.h"
#include "CutterDockWidget.h"

class MainWindow;

namespace Ui {
class ThreadsWidget;
}

class ThreadModel : public QAbstractListModel
{
    Q_OBJECT

    friend class ThreadsWidget;

private:
    QList<ThreadDescription> threads;

public:
    enum ColumnIndex {
        COLUMN_PID = 0,
        COLUMN_STATUS,
        COLUMN_PATH,
        COLUMN_PC,
        COLUMN_TLS,

        COLUMN_COUNT,
    };

    ThreadModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    void setList(QList<ThreadDescription> data);

    const ThreadDescription *description(const QModelIndex &index) const;
    QString translateStatus(const char status) const;
};

class ThreadsWidget : public CutterDockWidget
{
    Q_OBJECT
public:
    explicit ThreadsWidget(MainWindow *main);
    ~ThreadsWidget();

private slots:
    void updateContents();
    void setThreadsGrid();
    void fontsUpdatedSlot();
    void onActivated(const QModelIndex &index);
    void tableCustomContextMenu(const QPoint &pos);
    void onCurrentChanged(const QModelIndex &current, const QModelIndex &previous);

private:
    std::unique_ptr<Ui::ThreadsWidget> ui;
    ThreadModel *modelThreads;
    QSortFilterProxyModel *modelFilter;
    RefreshDeferrer *refreshDeferrer;
    QAction menuText;
    AddressableItemContextMenu addressableItemContextMenu;
};
