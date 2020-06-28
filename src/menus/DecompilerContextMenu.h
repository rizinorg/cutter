#ifndef DECOMPILERCONTEXTMENU_H
#define DECOMPILERCONTEXTMENU_H

#include "core/Cutter.h"
#include <QMenu>
#include <QKeySequence>

class DecompilerContextMenu : public QMenu
{
    Q_OBJECT

public:
    DecompilerContextMenu(QWidget *parent, MainWindow *mainWindow);
    ~DecompilerContextMenu();

    bool getIsTogglingBreakpoints();

signals:
    void copy();

public slots:
    void setOffset(RVA offset);
    void setCanCopy(bool enabled);
    void setFirstOffsetInLine(RVA firstOffset);
    void setAvailableBreakpoints(QVector<RVA> offsetList);


private slots:
    void aboutToShowSlot();

    void actionCopyTriggered();

    void actionToggleBreakpointTriggered();
    void actionAdvancedBreakpointTriggered();

    void actionContinueUntilTriggered();
    void actionSetPCTriggered();

private:
    // Private variables
    RVA offset;
    RVA firstOffsetInLine;
    bool isTogglingBreakpoints;
    QVector<RVA> availableBreakpoints;
    MainWindow *mainWindow;

    QAction actionCopy;
    QAction *copySeparator;

    QMenu *breakpointMenu;
    QAction actionToggleBreakpoint;
    QAction actionAdvancedBreakpoint;

    QMenu *breakpointsInLineMenu;

    QMenu *debugMenu;
    QAction actionContinueUntil;
    QAction actionSetPC;

    // Private Functions
    void setShortcutContextInActions(QMenu *menu);
    void setupBreakpointsInLineMenu();
    void setIsTogglingBreakpoints(bool isToggling);

    // Set actions
    void setActionCopy();

    void setActionToggleBreakpoint();
    void setActionAdvancedBreakpoint();

    void setActionContinueUntil();
    void setActionSetPC();

    // Add Menus
    void addBreakpointMenu();
    void addDebugMenu();
    // I left out the following part from RAnnotatedCode. Probably, we will be returning/passing annotations
    // from/to the function getThingUsedHere() and updateTargetMenuActions(). This block of comment will get removed in
    // future PRs.
    //
    // struct ThingUsedHere {
    //     QString name;
    //     RVA offset;
    //     enum class Type {
    //         Var,
    //         Function,
    //         Flag,
    //         Address
    //     };
    //     Type type;
    // };
    // QVector<ThingUsedHere> getThingUsedHere(RVA offset);

    // void updateTargetMenuActions(const QVector<ThingUsedHere> &targets);
};

#endif // DECOMPILERCONTEXTMENU_H