#pragma once

#include <QJsonObject>
#include <memory>
#include <QStandardItem>
#include <QTableView>

#include "Cutter.h"
#include "CutterDockWidget.h"

class MainWindow;

namespace Ui {
class StackWidget;
}

class StackWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit StackWidget(MainWindow *main, QAction *action = nullptr);
    ~StackWidget();

private slots:
    void updateContents();
    void setStackGrid();

private:
    std::unique_ptr<Ui::StackWidget> ui;
    QTableView *viewStack = new QTableView;
    QStandardItemModel *modelStack = new QStandardItemModel(1, 3, this);
};