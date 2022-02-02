#ifndef YARA_VIEW_MENU_H
#define YARA_VIEW_MENU_H

#include <core/Cutter.h>
#include <QMenu>
#include <QKeySequence>

#include "YaraDescription.h"
#include "YaraAddMetaDialog.h"

class MainWindow;

class YaraViewMenu : public QMenu
{
    Q_OBJECT

public:
    YaraViewMenu(QWidget *parent, MainWindow *mainWindow);
    virtual ~YaraViewMenu() {};

public slots:
    void setYaraTarget(const YaraDescription &description, bool remove);
    void setMetaTarget(const MetadataDescription &description);
    void clearTarget();

private:
    void onActionAddNewMetadata();
    void onActionCopyName();
    void onActionSeekAt();
    void onActionRemove();
    void onActionRemoveAll();

    QMenu *pluginMenu;
    QAction *pluginMenuAction;
    MainWindow *mainWindow;

    bool hasYaraTarget = false;
    bool hasMetaTarget = false;
    bool canRemove = false;

protected:
    QAction *actionAddNewMetadata;
    QAction *actionCopyName;
    QAction *actionSeekAt;
    QAction *actionRemove;
    QAction *actionRemoveAll;

    YaraDescription target_yara;
    MetadataDescription target_meta;
};
#endif // YARA_VIEW_MENU_H
