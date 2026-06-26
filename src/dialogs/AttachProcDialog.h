#pragma once

#include "core/Cutter.h"

#include <QAbstractListModel>
#include <QDialog>
#include <QSortFilterProxyModel>
#include <QTimer>

#include <memory>

namespace Ui {
class AttachProcDialog;
}

class MainWindow;
class QTreeWidget;
class QTreeWidgetItem;

class ProcessModel : public QAbstractListModel
{
    Q_OBJECT

private:
    QList<ProcessDescription> processes;

public:
    enum Column { PidColumn = 0, UidColumn, StatusColumn, PathColumn, ColumnCount };
    enum Role { ProcDescriptionRole = Qt::UserRole };

    ProcessModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    static bool lessThan(const ProcessDescription &left, const ProcessDescription &right,
                         int column);

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

/**
 * @brief Dialog for selecting and attaching to a running process for debugging
 */
class AttachProcDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AttachProcDialog(QWidget *parent = nullptr);
    ~AttachProcDialog();

    int getPID();

private slots:
    void onButtonBoxAccepted();
    void onButtonBoxRejected();
    void onAllProcViewDoubleClicked(const QModelIndex &index);
    void onProcBeingAnalyzedViewDoubleClicked(const QModelIndex &index);
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
