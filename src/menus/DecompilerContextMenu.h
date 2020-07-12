#ifndef DECOMPILERCONTEXTMENU_H
#define DECOMPILERCONTEXTMENU_H

#include "core/Cutter.h"
#include <QMenu>
#include <QKeySequence>

#include <r_util/r_annotated_code.h>

class DecompilerContextMenu : public QMenu
{
    Q_OBJECT

public:
    DecompilerContextMenu(QWidget *parent, MainWindow *mainWindow);
    ~DecompilerContextMenu();

    bool getIsTogglingBreakpoints();
    void setAnnotationHere(RCodeAnnotation *annotation);

signals:
    void copy();

public slots:
    void setOffset(RVA offset);
    void setCanCopy(bool enabled);
    void setFirstOffsetInLine(RVA firstOffset);
    void setAvailableBreakpoints(QVector<RVA> offsetList);


private slots:
    void aboutToShowSlot();
    void aboutToHideSlot();

    void actionCopyTriggered();

    void actionAddCommentTriggered();
    void actionDeleteCommentTriggered();

    void actionRenameThingHereTriggered();

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

    RCodeAnnotation *annotationHere;

    QAction actionCopy;
    QAction *copySeparator;

    QAction actionAddComment;
    QAction actionDeleteComment;

    QAction actionRenameThingHere;

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

    void setActionAddComment();
    void setActionDeleteComment();

    void setActionRenameThingHere();

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