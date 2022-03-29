#ifndef FLIRT_CONTEXTMENU_H
#define FLIRT_CONTEXTMENU_H

#include "core/Cutter.h"
#include <QMenu>
#include <QKeySequence>

class MainWindow;

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
    void onActionCopyLine();
    void onActionApplySignature();

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
