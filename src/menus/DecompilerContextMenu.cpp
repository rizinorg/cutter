#include "DecompilerContextMenu.h"
#include "dialogs/preferences/PreferencesDialog.h"
#include "MainWindow.h"
#include "dialogs/BreakpointsDialog.h"

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
        actionToggleBreakpoint(tr("Add/remove breakpoint"), this),
        actionAdvancedBreakpoint(tr("Advanced breakpoint"), this),
        breakpointsInLineMenu(new QMenu(this)),
        actionContinueUntil(tr("Continue until line"), this),
        actionSetPC(tr("Set PC"), this)
{
    setActionCopy();
    addSeparator();

    addBreakpointMenu();
    addDebugMenu();

    setShortcutContextInActions(this);

    connect(this, &DecompilerContextMenu::aboutToShow,
            this, &DecompilerContextMenu::aboutToShowSlot);
}

DecompilerContextMenu::~DecompilerContextMenu()
{
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

void DecompilerContextMenu::aboutToShowSlot()
{

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
}

// Set up actions

void DecompilerContextMenu::setActionCopy()
{
    connect(&actionCopy, &QAction::triggered, this, &DecompilerContextMenu::actionCopyTriggered);
    addAction(&actionCopy);
    actionCopy.setShortcut(QKeySequence::Copy);
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
