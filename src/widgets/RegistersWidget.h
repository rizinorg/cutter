#ifndef REGISTERSWIDGET_H
#define REGISTERSWIDGET_H

#include "CutterDockWidget.h"
#include "menus/AddressableItemContextMenu.h"

#include <QGridLayout>
#include <QJsonObject>
#include <QPlainTextEdit>
#include <QTextEdit>

#include <memory>

class MainWindow;

namespace Ui {
class RegistersWidget;
}

/**
 * @brief Dock widget that displays CPU registers in an editable grid for debugging/emulating
 */
class RegistersWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit RegistersWidget(MainWindow *main);
    ~RegistersWidget();

private slots:
    void updateContents();
    void setRegisterGrid();
    void openContextMenu(QPoint point, const QString &address);

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

#endif // REGISTERSWIDGET_H
