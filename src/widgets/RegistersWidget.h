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

    /**
     * @brief Opens the RegisterProfileDialog to view or edit register profile
     *
     * Updates Core with the new profile and saves the custom path to recent settings if the user
     * accepts the changes
     */
    void configureRegProfileClicked();

private:
    std::unique_ptr<Ui::RegistersWidget> ui;
    QGridLayout *registerLayout = new QGridLayout;
    AddressableItemContextMenu addressContextMenu;
    int numCols = 2;
    int registerLen = 0;
    RefreshDeferrer *refreshDeferrer;
    QString currProfilePath;
};
