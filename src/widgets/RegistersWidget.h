#pragma once

#include <QTextEdit>
#include <QPlainTextEdit>
#include <QGridLayout>
#include <QJsonObject>
#include <memory>

#include "core/Cutter.h"
#include "CutterDockWidget.h"
#include "menus/AddressableItemContextMenu.h"

class MainWindow;

namespace Ui {
class RegistersWidget;
}

class RegistersWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit RegistersWidget(MainWindow *main);
    ~RegistersWidget();

private slots:
    void updateContents();
    void setRegisterGrid();
    void openContextMenu(QPoint point, QString address);

private:
    std::unique_ptr<Ui::RegistersWidget> ui;
    QGridLayout *registerLayout = new QGridLayout;
    AddressableItemContextMenu addressContextMenu;
    int numCols = 2;
    int registerLen = 0;
    RefreshDeferrer *refreshDeferrer;
};
