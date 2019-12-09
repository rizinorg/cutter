#pragma once

#include <QJsonObject>
#include <memory>
#include <QStandardItem>
#include <QTableView>
#include <QSortFilterProxyModel>

#include "core/Cutter.h"
#include "CutterDockWidget.h"

class MainWindow;

namespace Ui {
class ProcessesWidget;
}

class ProcessesFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    ProcessesFilterModel(QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
};

class ProcessesWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit ProcessesWidget(MainWindow *main, QAction *action = nullptr);
    ~ProcessesWidget();

private slots:
    void updateContents();
    void setProcessesGrid();
    void fontsUpdatedSlot();
    void onActivated(const QModelIndex &index);

private:
    QString translateStatus(QString status);
    std::unique_ptr<Ui::ProcessesWidget> ui;
    QStandardItemModel *modelProcesses;
    ProcessesFilterModel *modelFilter;
    RefreshDeferrer *refreshDeferrer;
};
