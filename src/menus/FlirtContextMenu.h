#ifndef FLIRT_CONTEXTMENU_H
#define FLIRT_CONTEXTMENU_H

#include "CutterDescriptions.h"

#include <QKeySequence>
#include <QMenu>

class MainWindow;

/**
 * @brief Context menu for @ref FlirtWidget
 */
class CUTTER_EXPORT FlirtContextMenu : public QMenu
{
    Q_OBJECT

public:
    FlirtContextMenu(QWidget *parent, MainWindow *mainWindow);
    ~FlirtContextMenu();

public slots:
    void setTarget(const FlirtDescription &flirt);
    void clearTarget();

private:
    void onActionCopyLine() const;
    void onActionApplySignature() const;

    QMenu *pluginMenu;
    QAction *pluginMenuAction;
    MainWindow *mainWindow;

    bool hasTarget = false;

protected:
    void setHasTarget(bool hasTarget);
    QAction *actionApplySignature;
    QAction *actionCopyLine;

    FlirtDescription entry;
};
#endif // FLIRT_CONTEXTMENU_H
