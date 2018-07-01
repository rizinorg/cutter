#pragma once

#include "Cutter.h"
#include <QDialog>
#include <memory>
#include <QAbstractListModel>
#include <QSortFilterProxyModel>

namespace Ui {
class AttachProcDialog;
}

class MainWindow;
class QTreeWidget;
class QTreeWidgetItem;


class ProcessModel: public QAbstractListModel
{
    Q_OBJECT

private:
    QList<ProcessDescription> *processes;

public:
    enum Column { PidColumn = 0, UidColumn, StatusColumn, PathColumn, ColumnCount };
    enum Role { ProcDescriptionRole = Qt::UserRole };

    ProcessModel(QList<ProcessDescription> *processes, QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    void beginReloadProcess();
    void endReloadProcess();
};



class ProcessProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    ProcessProxyModel(ProcessModel *sourceModel, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};



class AttachProcDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AttachProcDialog(QWidget *parent = nullptr);
    ~AttachProcDialog();

    int getPID();

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
    void on_procTreeView_doubleClicked(const QModelIndex &index);

signals:
    void attachProcess(int pid);
private:
    std::unique_ptr<Ui::AttachProcDialog> ui;
    bool eventFilter(QObject *obj, QEvent *event);

    ProcessModel *processModel;
    ProcessProxyModel *processProxyModel;
    QList<ProcessDescription> processes;

};
