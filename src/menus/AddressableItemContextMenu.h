#ifndef ADDRESSABLEITEMCONTEXTMENU_H
#define ADDRESSABLEITEMCONTEXTMENU_H

#include "CutterCommon.h"

#include <QKeySequence>
#include <QMenu>

class MainWindow;

/**
 * @brief Generic context menu for Addressable widgets
 */
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
    void onActionCopyAddress() const;
    void onActionShowXrefs();
    void onActionAddComment();
    void onActionToggleBreakpoint() const;

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
