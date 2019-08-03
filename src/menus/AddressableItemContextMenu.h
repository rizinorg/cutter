#ifndef ADDRESSABLEITEMCONTEXTMENU_H
#define ADDRESSABLEITEMCONTEXTMENU_H

#include "core/Cutter.h"
#include <QMenu>
#include <QKeySequence>

class AddressableItemContextMenu : public QMenu
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
private:
    void onActionCopyAddress();
    void onActionShowXrefs();

    virtual void aboutToShowSlot();

    MainWindow *mainWindow;

    RVA offset;
protected:
    QAction actionShowInMenu;
    QAction actionCopyAddress;
    QAction actionShowXrefs;

    QString name;
    bool wholeFunction = false;
};
#endif // ADDRESSABLEITEMCONTEXTMENU_H
