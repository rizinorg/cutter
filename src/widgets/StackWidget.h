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

class StackWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit StackWidget(MainWindow *main);
    ~StackWidget();

private slots:
    void updateContents();
    void setStackGrid();

private:
    std::unique_ptr<Ui::StackWidget> ui;
    QTableView *viewStack = new QTableView;
    QStandardItemModel *modelStack = new QStandardItemModel(1, 3, this);
};