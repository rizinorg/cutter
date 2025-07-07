#include "AddressableItemContextMenu.h"
#include "dialogs/XrefsDialog.h"
#include "MainWindow.h"
#include "dialogs/CommentsDialog.h"
#include "shortcuts/ShortcutManager.h"

#include <QtCore>
#include <QShortcut>
#include <QJsonArray>
#include <QClipboard>
#include <QApplication>
#include <QPushButton>

AddressableItemContextMenu::AddressableItemContextMenu(QWidget *parent, MainWindow *mainWindow)
    : QMenu(parent), mainWindow(mainWindow)
{
    actionShowInMenu = new QAction(tr("Show in"), this);
    actionCopyAddress = Shortcuts()->makeAction("General.copyAddress", this);
    actionShowXrefs = Shortcuts()->makeAction("General.showXRefs", this);
    actionAddComment = Shortcuts()->makeAction("General.addComment", this);
    actionToggleBreakpoint = Shortcuts()->makeAction("Debug.toggleBreakpoint", this);

    connect(actionCopyAddress, &QAction::triggered, this,
            &AddressableItemContextMenu::onActionCopyAddress);
    actionCopyAddress->setShortcutContext(Qt::ShortcutContext::WidgetWithChildrenShortcut);

    connect(actionShowXrefs, &QAction::triggered, this,
            &AddressableItemContextMenu::onActionShowXrefs);
    actionShowXrefs->setShortcutContext(Qt::ShortcutContext::WidgetWithChildrenShortcut);

    connect(actionAddComment, &QAction::triggered, this,
            &AddressableItemContextMenu::onActionAddComment);
    actionAddComment->setShortcutContext(Qt::ShortcutContext::WidgetWithChildrenShortcut);

    connect(actionToggleBreakpoint, &QAction::triggered, this,
            &AddressableItemContextMenu::onActionToggleBreakpoint);
    actionToggleBreakpoint->setShortcutContext(Qt::ShortcutContext::WidgetWithChildrenShortcut);

    addAction(actionShowInMenu);
    addAction(actionCopyAddress);
    addAction(actionShowXrefs);
    addSeparator();
    addAction(actionAddComment);
    addAction(actionToggleBreakpoint);

    addSeparator();
    pluginMenu = mainWindow->getContextMenuExtensions(MainWindow::ContextMenuType::Addressable);
    pluginMenuAction = addMenu(pluginMenu);
    addSeparator();

    setHasTarget(hasTarget);

    connect(this, &QMenu::aboutToShow, this, &AddressableItemContextMenu::aboutToShowSlot);
}

AddressableItemContextMenu::~AddressableItemContextMenu() {}

void AddressableItemContextMenu::setWholeFunction(bool wholeFunciton)
{
    this->wholeFunction = wholeFunciton;
}

void AddressableItemContextMenu::setOffset(RVA offset)
{
    setTarget(offset);
}

void AddressableItemContextMenu::setTarget(RVA offset, QString name)
{
    this->offset = offset;
    this->name = name;
    setHasTarget(true);
}

void AddressableItemContextMenu::clearTarget()
{
    setHasTarget(false);
}

void AddressableItemContextMenu::toggleBreakpointAction(bool enabled)
{
    breakpointActionEnabled = enabled;
    // Update actionToggleBreakpoint visibility
    setHasTarget(hasTarget);
}

void AddressableItemContextMenu::onActionCopyAddress()
{
    auto clipboard = QApplication::clipboard();
    clipboard->setText(RzAddressString(offset));
}

void AddressableItemContextMenu::onActionShowXrefs()
{
    emit xrefsTriggered();
    XrefsDialog dialog(mainWindow, true);
    QString tmpName = name;
    if (name.isEmpty()) {
        name = RzAddressString(offset);
    }
    dialog.fillRefsForAddress(offset, name, wholeFunction);
    dialog.exec();
}

void AddressableItemContextMenu::onActionAddComment()
{
    CommentsDialog::addOrEditComment(offset, this);
}

void AddressableItemContextMenu::onActionToggleBreakpoint()
{
    Core()->toggleBreakpoint(offset);
}

void AddressableItemContextMenu::aboutToShowSlot()
{
    if (Core()->getCommentAt(offset).isEmpty()) {
        actionAddComment->setText(tr("Add Comment"));
    } else {
        actionAddComment->setText(tr("Edit Comment"));
    }

    if (Core()->breakpointIndexAt(offset) < 0) {
        actionToggleBreakpoint->setText(tr("Add Breakpoint"));
    } else {
        actionToggleBreakpoint->setText(tr("Remove Breakpoint"));
    }

    if (actionShowInMenu->menu()) {
        actionShowInMenu->menu()->deleteLater();
    }
    actionShowInMenu->setMenu(mainWindow->createShowInMenu(this, offset));

    pluginMenuAction->setVisible(!pluginMenu->isEmpty());
    for (QAction *pluginAction : pluginMenu->actions()) {
        pluginAction->setData(QVariant::fromValue(offset));
    }
}

void AddressableItemContextMenu::setHasTarget(bool hasTarget)
{
    this->hasTarget = hasTarget;
    actionShowInMenu->setEnabled(hasTarget);
    actionCopyAddress->setEnabled(hasTarget);
    actionShowXrefs->setEnabled(hasTarget);
    actionAddComment->setEnabled(hasTarget);
    actionToggleBreakpoint->setEnabled(hasTarget && breakpointActionEnabled);
    actionToggleBreakpoint->setVisible(hasTarget && breakpointActionEnabled);
}
