#pragma once

#include <QJsonObject>
#include <memory>
#include <QStandardItem>
#include <QTableView>

#include "core/Cutter.h"
#include "CutterDockWidget.h"

class MainWindow;

namespace Ui {
class ThreadsWidget;
}

class ThreadsWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit ThreadsWidget(MainWindow *main, QAction *action = nullptr);
    ~ThreadsWidget();

private slots:
    void updateContents();
    void setThreadsGrid();
    void fontsUpdatedSlot();
    void onDoubleClicked(const QModelIndex &index);

private:
    std::unique_ptr<Ui::ThreadsWidget> ui;
    QStandardItemModel *modelThreads = new QStandardItemModel(1, 4, this);
    QTableView *viewThreads = new QTableView;
    RefreshDeferrer *refreshDeferrer;
};
