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
        mainWindow(mainWindow),
        actionCopy(tr("Copy"), this),
        actionAddBreakpoint(tr("Add/remove breakpoint"), this),
        actionAdvancedBreakpoint(tr("Advanced breakpoint"), this)
{
    setActionCopy();
    addSeparator();

    addBreakpointMenu();

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

void DecompilerContextMenu::setCanCopy(bool enabled)
{
    actionCopy.setVisible(enabled);
}

void DecompilerContextMenu::aboutToShowSlot()
{
    bool hasBreakpoint = Core()->breakpointIndexAt(offset) > -1;
    actionAddBreakpoint.setText(hasBreakpoint ?
                                     tr("Remove breakpoint") : tr("Add breakpoint"));
    actionAdvancedBreakpoint.setText(hasBreakpoint ?
                                     tr("Edit breakpoint") : tr("Advanced breakpoint"));
    QString progCounterName = Core()->getRegisterName("PC").toUpper();
    actionSetPC.setText("Set " + progCounterName + " here");
}

// Set up actions

void DecompilerContextMenu::setActionCopy(){
    connect(&actionCopy, &QAction::triggered, this, &DecompilerContextMenu::actionCopyTriggered);
    addAction(&actionCopy);
    actionCopy.setShortcut(QKeySequence::Copy);
    actionCopy.setShortcutContext(Qt::WidgetWithChildrenShortcut);
}

void DecompilerContextMenu::setActionAddBreakpoint()
{
    connect(&actionAddBreakpoint, &QAction::triggered, this, &DecompilerContextMenu::actionAddBreakpointTriggered);
    actionAddBreakpoint.setShortcuts({Qt::Key_F2, Qt::CTRL + Qt::Key_B});
    actionAddBreakpoint.setShortcutContext(Qt::WidgetWithChildrenShortcut);
}

void DecompilerContextMenu::setActionAdvancedBreakpoint()
{
    connect(&actionAdvancedBreakpoint, &QAction::triggered, this, &DecompilerContextMenu::actionAdvancedBreakpointTriggered);
    actionAdvancedBreakpoint.setShortcut({Qt::CTRL + Qt::Key_F2});
    actionAdvancedBreakpoint.setShortcutContext(Qt::WidgetWithChildrenShortcut);
}
// Set up action responses

void DecompilerContextMenu::actionCopyTriggered()
{
    emit copy();
}

void DecompilerContextMenu::actionAddBreakpointTriggered()
{
    Core()->toggleBreakpoint(offset);
}

void DecompilerContextMenu::actionAdvancedBreakpointTriggered()
{
    int index = Core()->breakpointIndexAt(offset);
    if (index >= 0) {
        BreakpointsDialog::editBreakpoint(Core()->getBreakpointAt(offset), this);
    } else {
        BreakpointsDialog::createNewBreakpoint(offset, this);
    }
}

// Set up menus

void DecompilerContextMenu::addBreakpointMenu()
{
    breakpointMenu = addMenu(tr("Breakpoint"));

    setActionAddBreakpoint();
    breakpointMenu->addAction(&actionAddBreakpoint);
    setActionAdvancedBreakpoint();
    breakpointMenu->addAction(&actionAdvancedBreakpoint);
}
