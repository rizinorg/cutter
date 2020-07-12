#include "DecompilerContextMenu.h"
#include "dialogs/preferences/PreferencesDialog.h"
#include "MainWindow.h"
#include "dialogs/BreakpointsDialog.h"
#include "dialogs/CommentsDialog.h"
#include "dialogs/RenameDialog.h"

#include <QtCore>
#include <QShortcut>
#include <QJsonArray>
#include <QClipboard>
#include <QApplication>
#include <QPushButton>

DecompilerContextMenu::DecompilerContextMenu(QWidget *parent, MainWindow *mainWindow)
    :   QMenu(parent),
        offset(0),
        isTogglingBreakpoints(false),
        mainWindow(mainWindow),
        actionCopy(tr("Copy"), this),
        actionAddComment(tr("Add Comment"), this),
        actionDeleteComment(tr("Delete comment"), this),
        actionRenameThingHere(tr("Rename function at cursor"), this),
        actionToggleBreakpoint(tr("Add/remove breakpoint"), this),
        actionAdvancedBreakpoint(tr("Advanced breakpoint"), this),
        breakpointsInLineMenu(new QMenu(this)),
        actionContinueUntil(tr("Continue until line"), this),
        actionSetPC(tr("Set PC"), this)
{
    setActionCopy();
    addSeparator();

    setActionAddComment();
    setActionDeleteComment();

    setActionRenameThingHere();

    addSeparator();
    addBreakpointMenu();
    addDebugMenu();

    setShortcutContextInActions(this);

    connect(this, &DecompilerContextMenu::aboutToShow,
            this, &DecompilerContextMenu::aboutToShowSlot);
    connect(this, &DecompilerContextMenu::aboutToHide,
            this, &DecompilerContextMenu::aboutToHideSlot);
}

DecompilerContextMenu::~DecompilerContextMenu()
{
}

void DecompilerContextMenu::setAnnotationHere(RCodeAnnotation *annotation)
{
    this->annotationHere = annotation;
}

void DecompilerContextMenu::setOffset(RVA offset)
{
    this->offset = offset;

    // this->actionSetFunctionVarTypes.setVisible(true);
}

void DecompilerContextMenu::setFirstOffsetInLine(RVA firstOffset)
{
    this->firstOffsetInLine = firstOffset;
}

void DecompilerContextMenu::setAvailableBreakpoints(QVector<RVA> offsetList)
{
    this->availableBreakpoints = offsetList;
}

void DecompilerContextMenu::setupBreakpointsInLineMenu()
{
    breakpointsInLineMenu->clear();
    for (auto curOffset : this->availableBreakpoints) {
        QAction *action = breakpointsInLineMenu->addAction(RAddressString(curOffset));
        connect(action, &QAction::triggered, this, [this, curOffset] {
            BreakpointsDialog::editBreakpoint(Core()->getBreakpointAt(curOffset),
                                              this);
        });
    }
}

void DecompilerContextMenu::setCanCopy(bool enabled)
{
    actionCopy.setVisible(enabled);
}

void DecompilerContextMenu::setShortcutContextInActions(QMenu *menu)
{
    for (QAction *action : menu->actions()) {
        if (action->isSeparator()) {
            //Do nothing
        } else if (action->menu()) {
            setShortcutContextInActions(action->menu());
        } else {
            action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        }
    }
}

void DecompilerContextMenu::setIsTogglingBreakpoints(bool isToggling)
{
    this->isTogglingBreakpoints = isToggling;
}

bool DecompilerContextMenu::getIsTogglingBreakpoints()
{
    return this->isTogglingBreakpoints;
}

void DecompilerContextMenu::aboutToHideSlot()
{
    actionAddComment.setVisible(true);
}

void DecompilerContextMenu::aboutToShowSlot()
{
    if (this->firstOffsetInLine != RVA_MAX) {
        QString comment = Core()->cmdRawAt("CC.", this->firstOffsetInLine);
        actionAddComment.setVisible(true);
        if (comment.isEmpty()) {
            actionDeleteComment.setVisible(false);
            actionAddComment.setText(tr("Add Comment"));
        } else {
            actionDeleteComment.setVisible(true);
            actionAddComment.setText(tr("Edit Comment"));
        }
    } else {
        actionAddComment.setVisible(false);
        actionDeleteComment.setVisible(false);
    }


    setupBreakpointsInLineMenu();

    // Only show debug options if we are currently debugging
    debugMenu->menuAction()->setVisible(Core()->currentlyDebugging);

    bool hasBreakpoint = !this->availableBreakpoints.isEmpty();
    int numberOfBreakpoints = this->availableBreakpoints.size();
    if (numberOfBreakpoints == 0) {
        actionToggleBreakpoint.setText(tr("Add breakpoint"));
    } else if (numberOfBreakpoints == 1) {
        actionToggleBreakpoint.setText(tr("Remove breakpoint"));
    } else {
        actionToggleBreakpoint.setText(tr("Remove all breakpoints in line"));
    }

    if (numberOfBreakpoints > 1) {
        actionAdvancedBreakpoint.setMenu(breakpointsInLineMenu);
    } else {
        actionAdvancedBreakpoint.setMenu(nullptr);
    }
    actionAdvancedBreakpoint.setText(hasBreakpoint ?
                                     tr("Edit breakpoint") : tr("Advanced breakpoint"));

    QString progCounterName = Core()->getRegisterName("PC").toUpper();
    actionSetPC.setText(tr("Set %1 here").arg(progCounterName));

    if (!annotationHere) { // To be considered as invalid
        actionRenameThingHere.setVisible(false);
    } else {
        actionRenameThingHere.setVisible(true);
        actionRenameThingHere.setText(tr("Rename function %1").arg(QString(
                                                                       annotationHere->function_name.name)));
    }
}

// Set up actions

void DecompilerContextMenu::setActionCopy()
{
    connect(&actionCopy, &QAction::triggered, this, &DecompilerContextMenu::actionCopyTriggered);
    addAction(&actionCopy);
    actionCopy.setShortcut(QKeySequence::Copy);
}

void DecompilerContextMenu::setActionAddComment()
{
    connect(&actionAddComment, &QAction::triggered, this,
            &DecompilerContextMenu::actionAddCommentTriggered);
    addAction(&actionAddComment);
    actionAddComment.setShortcut(Qt::Key_Semicolon);
}

void DecompilerContextMenu::setActionDeleteComment()
{
    connect(&actionDeleteComment, &QAction::triggered, this,
            &DecompilerContextMenu::actionDeleteCommentTriggered);
    addAction(&actionDeleteComment);
}

void DecompilerContextMenu::setActionRenameThingHere()
{
    actionRenameThingHere.setShortcut({Qt::SHIFT + Qt::Key_N});
    connect(&actionRenameThingHere, &QAction::triggered, this,
            &DecompilerContextMenu::actionRenameThingHereTriggered);
    addAction(&actionRenameThingHere);
}

void DecompilerContextMenu::setActionToggleBreakpoint()
{
    connect(&actionToggleBreakpoint, &QAction::triggered, this,
            &DecompilerContextMenu::actionToggleBreakpointTriggered);
    actionToggleBreakpoint.setShortcuts({Qt::Key_F2, Qt::CTRL + Qt::Key_B});
}

void DecompilerContextMenu::setActionAdvancedBreakpoint()
{
    connect(&actionAdvancedBreakpoint, &QAction::triggered, this,
            &DecompilerContextMenu::actionAdvancedBreakpointTriggered);
    actionAdvancedBreakpoint.setShortcut({Qt::CTRL + Qt::Key_F2});
}

void DecompilerContextMenu::setActionContinueUntil()
{
    connect(&actionContinueUntil, &QAction::triggered, this,
            &DecompilerContextMenu::actionContinueUntilTriggered);
}

void DecompilerContextMenu::setActionSetPC()
{
    connect(&actionSetPC, &QAction::triggered, this, &DecompilerContextMenu::actionSetPCTriggered);
}

// Set up action responses

void DecompilerContextMenu::actionCopyTriggered()
{
    emit copy();
}

void DecompilerContextMenu::actionAddCommentTriggered()
{
    CommentsDialog::addOrEditComment(this->firstOffsetInLine, this);
}

void DecompilerContextMenu::actionDeleteCommentTriggered()
{
    Core()->delComment(this->firstOffsetInLine);
}

void DecompilerContextMenu::actionRenameThingHereTriggered()
{
    if (!annotationHere)
        return;

    RenameDialog dialog(mainWindow);
    auto type = annotationHere->type;
    if (type == R_CODE_ANNOTATION_TYPE_FUNCTION_NAME) {
        QString currentName(annotationHere->function_name.name);
        dialog.setWindowTitle(tr("Rename function %1").arg(currentName));
        dialog.setName(currentName);
    }
    if (dialog.exec()) {
        QString newName = dialog.getName();
        if (!newName.isEmpty()) {
            if (type == R_CODE_ANNOTATION_TYPE_FUNCTION_NAME) {
                Core()->renameFunction(annotationHere->function_name.offset, newName);
            }
        }
    }
}

void DecompilerContextMenu::actionToggleBreakpointTriggered()
{
    if (!this->availableBreakpoints.isEmpty()) {
        setIsTogglingBreakpoints(true);
        for (auto offsetToRemove : this->availableBreakpoints) {
            Core()->toggleBreakpoint(offsetToRemove);
        }
        this->availableBreakpoints.clear();
        setIsTogglingBreakpoints(false);
        return;
    }
    if (this->firstOffsetInLine == RVA_MAX)
        return;

    Core()->toggleBreakpoint(this->firstOffsetInLine);
}

void DecompilerContextMenu::actionAdvancedBreakpointTriggered()
{
    if (!availableBreakpoints.empty()) {
        // Edit the earliest breakpoint in the line
        BreakpointsDialog::editBreakpoint(Core()->getBreakpointAt(this->availableBreakpoints.first()),
                                          this);
    } else {
        // Add a breakpoint to the earliest offset in the line
        BreakpointsDialog::createNewBreakpoint(this->firstOffsetInLine, this);
    }
}

void DecompilerContextMenu::actionContinueUntilTriggered()
{
    Core()->continueUntilDebug(RAddressString(offset));
}

void DecompilerContextMenu::actionSetPCTriggered()
{
    QString progCounterName = Core()->getRegisterName("PC");
    Core()->setRegister(progCounterName, RAddressString(offset).toUpper());
}

// Set up menus

void DecompilerContextMenu::addBreakpointMenu()
{
    breakpointMenu = addMenu(tr("Breakpoint"));

    setActionToggleBreakpoint();
    breakpointMenu->addAction(&actionToggleBreakpoint);
    setActionAdvancedBreakpoint();
    breakpointMenu->addAction(&actionAdvancedBreakpoint);
}

void DecompilerContextMenu::addDebugMenu()
{
    debugMenu = addMenu(tr("Debug"));

    setActionContinueUntil();
    debugMenu->addAction(&actionContinueUntil);
    setActionSetPC();
    debugMenu->addAction(&actionSetPC);
}
