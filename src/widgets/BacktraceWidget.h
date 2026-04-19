#pragma once

#include <QJsonObject>
#include <memory>
#include <QStandardItem>
#include <QTableView>

#include "core/Cutter.h"
#include "CutterDockWidget.h"

class MainWindow;

namespace Ui {
class BacktraceWidget;
}

class BacktraceModel : public QAbstractListModel
{
public:
    explicit BacktraceModel(QObject *parent = nullptr);

    enum Column { Function = 0, PC, SP, FrameSize, Description, Count };

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    void setBacktraces(const QList<BacktraceDescription> &backtraces);

private:
    QList<BacktraceDescription> backtraces;
};

class BacktraceWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit BacktraceWidget(MainWindow *main);
    ~BacktraceWidget();

private slots:
    void updateContents();
    void fontsUpdatedSlot();

private:
    std::unique_ptr<Ui::BacktraceWidget> ui;
    BacktraceModel *backtraceModel;
    QTreeView *backtraceView;
    RefreshDeferrer *refreshDeferrer;

    void adjustFunctionNameCol();
};
