#include "DecompilerContextMenu.h"
#include "dialogs/preferences/PreferencesDialog.h"
#include "MainWindow.h"

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
        actionCopy(tr("Copy"), this)
{
    setActionCopy();
    addSeparator();

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
}

// Set up actions

void DecompilerContextMenu::setActionCopy(){
    connect(&actionCopy, &QAction::triggered, this, &DecompilerContextMenu::actionCopyTriggered);
    addAction(&actionCopy);
    actionCopy.setShortcut(QKeySequence::Copy);
    actionCopy.setShortcutContext(Qt::WidgetWithChildrenShortcut);
}

// Set up action responses

void DecompilerContextMenu::actionCopyTriggered()
{
    emit copy();
}
