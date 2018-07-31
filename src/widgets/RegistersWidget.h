#pragma once

#include <QTextEdit>
#include <QPlainTextEdit>
#include <QGridLayout>
#include <QJsonObject>
#include <memory>

#include "Cutter.h"
#include "CutterDockWidget.h"

class MainWindow;

namespace Ui {
class RegistersWidget;
}

class RegistersWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit RegistersWidget(MainWindow *main, QAction *action = nullptr);
    ~RegistersWidget();

private slots:
    void updateContents();
    void setRegisterGrid();

private:
    std::unique_ptr<Ui::RegistersWidget> ui;
    QGridLayout *registerLayout = new QGridLayout;
    int numCols = 2;
    int registerLen = 0;
};