#pragma once

#include "Cutter.h"
#include <QDialog>
#include <memory>
#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QTimer>

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
    QList<ProcessDescription> processes;

public:
    enum Column { PidColumn = 0, UidColumn, StatusColumn, PathColumn, ColumnCount };
    enum Role { ProcDescriptionRole = Qt::UserRole };

    ProcessModel(QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    static bool lessThan(const ProcessDescription &left, const ProcessDescription &right, int column);

public slots:
    void updateData();
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


class ProcessBeingAnalysedProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    ProcessBeingAnalysedProxyModel(ProcessModel *sourceModel, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    QString processBeingAnalysedFilename;
    QString processPathToFilename(const QString &path) const;
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
    void on_allProcView_doubleClicked(const QModelIndex &index);
    void on_procBeingAnalyzedView_doubleClicked(const QModelIndex &index);
    void updateModelData();

private:
    std::unique_ptr<Ui::AttachProcDialog> ui;
    bool eventFilter(QObject *obj, QEvent *event);

    ProcessModel *processModel;
    ProcessProxyModel *processProxyModel;
    ProcessBeingAnalysedProxyModel *processBeingAnalyzedProxyModel;

    // whether the 'small table' or 'table with all procs' was last focused
    bool wasAllProcViewLastPressed = false;

    QTimer *timer;
    const int updateIntervalMs = 1000;
};
