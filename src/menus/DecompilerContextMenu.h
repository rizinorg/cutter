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
    void setCurHighlightedWord(QString word);
    void setOffset(RVA offset);
    void setDecompiledFunctionAddress(RVA functionAddr);
    void setFirstOffsetInLine(RVA firstOffset);
    void setAvailableBreakpoints(QVector<RVA> offsetList);


private slots:
    void aboutToShowSlot();
    void aboutToHideSlot();

    void actionCopyTriggered();
    void actionCopyInstructionAddressTriggered();
    void actionCopyReferenceAddressTriggered();

    void actionAddCommentTriggered();
    void actionDeleteCommentTriggered();

    void actionRenameThingHereTriggered();
    void actionDeleteNameTriggered();

    void actionEditFunctionVariablesTriggered();

    void actionXRefsTriggered();

    void actionToggleBreakpointTriggered();
    void actionAdvancedBreakpointTriggered();

    void actionContinueUntilTriggered();
    void actionSetPCTriggered();

private:
    // Private variables
    QString curHighlightedWord;
    RVA offset;
    RVA decompiledFunctionAddress;
    RVA firstOffsetInLine;
    bool isTogglingBreakpoints;
    QVector<RVA> availableBreakpoints;
    MainWindow *mainWindow;

    RCodeAnnotation *annotationHere;

    QAction actionCopy;
    QAction actionCopyInstructionAddress;
    QAction actionCopyReferenceAddress;
    QAction *copySeparator;

    QAction actionShowInSubmenu;
    QList<QAction *> showTargetMenuActions;

    QAction actionAddComment;
    QAction actionDeleteComment;

    QAction actionRenameThingHere;
    QAction actionDeleteName;

    QAction actionEditFunctionVariables;

    QAction actionXRefs;

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

    void setActionShowInSubmenu();

    void setActionAddComment();
    void setActionDeleteComment();

    void setActionXRefs();

    void setActionRenameThingHere();
    void setActionDeleteName();

    void setActionEditFunctionVariables();

    void setActionToggleBreakpoint();
    void setActionAdvancedBreakpoint();

    void setActionContinueUntil();
    void setActionSetPC();

    // Add Menus
    void addBreakpointMenu();
    void addDebugMenu();

    void updateTargetMenuActions();

    bool isFunctionVariable();
    bool variablePresentInR2();
};

#endif // DECOMPILERCONTEXTMENU_H