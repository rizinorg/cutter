#ifndef PROCESSESWIDGET_H
#define PROCESSESWIDGET_H

#include "CutterDockWidget.h"

#include <QJsonObject>
#include <QSortFilterProxyModel>
#include <QStandardItem>
#include <QTableView>

#include <memory>

class MainWindow;

namespace Ui {
class ProcessesWidget;
}

/**
 * @brief Filter model for @ref ProcessesWidget
 */
class ProcessesFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    ProcessesFilterModel(QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
};

/**
 * @brief Dock widget that lists system processes during debugging/emulation
 */
class ProcessesWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    enum ColumnIndex : ut8 {
        COLUMN_PID = 0,
        COLUMN_UID,
        COLUMN_STATUS,
        COLUMN_PATH,
    };

    explicit ProcessesWidget(MainWindow *main);
    ~ProcessesWidget();

private slots:
    void updateContents();
    void setProcessesGrid();
    void fontsUpdatedSlot();
    void onActivated(const QModelIndex &index);

private:
    QString translateStatus(const char status);
    std::unique_ptr<Ui::ProcessesWidget> ui;
    QStandardItemModel *modelProcesses;
    ProcessesFilterModel *modelFilter;
    RefreshDeferrer *refreshDeferrer;
};

#endif // PROCESSESWIDGET_H
