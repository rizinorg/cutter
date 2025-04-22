#ifndef ADDRESSABLEITEMCONTEXTMENU_H
#define ADDRESSABLEITEMCONTEXTMENU_H

#include "core/Cutter.h"
#include <QMenu>
#include <QKeySequence>

class MainWindow;

class CUTTER_EXPORT AddressableItemContextMenu : public QMenu
{
    Q_OBJECT

public:
    AddressableItemContextMenu(QWidget *parent, MainWindow *mainWindow);
    ~AddressableItemContextMenu();

    /**
     * @brief Configure if addressable item refers to whole function or specific address
     * @param wholeFunciton
     */
    void setWholeFunction(bool wholeFunciton);
public slots:
    void setOffset(RVA offset);
    void setTarget(RVA offset, QString name = QString());
    void clearTarget();
    void toggleBreakpointAction(bool enabled);
signals:
    void xrefsTriggered();

private:
    void onActionCopyAddress();
    void onActionShowXrefs();
    void onActionAddComment();
    void onActionToggleBreakpoint();

    virtual void aboutToShowSlot();

    QMenu *pluginMenu;
    QAction *pluginMenuAction;
    MainWindow *mainWindow;

    RVA offset;
    bool hasTarget = false;

protected:
    void setHasTarget(bool hasTarget);
    QAction *actionShowInMenu;
    QAction *actionCopyAddress;
    QAction *actionShowXrefs;
    QAction *actionAddComment;
    QAction *actionToggleBreakpoint;

    QString name;
    bool wholeFunction = false;
    bool breakpointActionEnabled = false;
};
#endif // ADDRESSABLEITEMCONTEXTMENU_H
