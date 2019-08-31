#include "AddressableItemContextMenu.h"
#include "dialogs/XrefsDialog.h"
#include "MainWindow.h"
#include "dialogs/CommentsDialog.h"

#include <QtCore>
#include <QShortcut>
#include <QJsonArray>
#include <QClipboard>
#include <QApplication>
#include <QPushButton>

AddressableItemContextMenu::AddressableItemContextMenu(QWidget *parent, MainWindow *mainWindow)
    : QMenu(parent)
    , mainWindow(mainWindow)
    , actionShowInMenu(tr("Show in"), this)
    , actionCopyAddress(tr("Copy address"), this)
    , actionShowXrefs(tr("Show X-Refs"), this)
    , actionAddcomment(tr("Add comment"), this)
{
    connect(&actionCopyAddress, &QAction::triggered, this,
            &AddressableItemContextMenu::onActionCopyAddress);
    actionCopyAddress.setShortcuts({Qt::CTRL + Qt::SHIFT + Qt::Key_C});
    actionCopyAddress.setShortcutContext(Qt::ShortcutContext::WidgetWithChildrenShortcut);

    connect(&actionShowXrefs, &QAction::triggered, this,
            &AddressableItemContextMenu::onActionShowXrefs);
    actionShowXrefs.setShortcut({Qt::Key_X});
    actionShowXrefs.setShortcutContext(Qt::ShortcutContext::WidgetWithChildrenShortcut);

    connect(&actionAddcomment, &QAction::triggered, this,
            &AddressableItemContextMenu::onActionAddComment);
    actionAddcomment.setShortcut({Qt::Key_Semicolon});
    actionAddcomment.setShortcutContext(Qt::ShortcutContext::WidgetWithChildrenShortcut);

    addAction(&actionShowInMenu);
    addAction(&actionCopyAddress);
    addAction(&actionShowXrefs);
    addSeparator();
    addAction(&actionAddcomment);

    setHasTarget(hasTarget);
    connect(this, &QMenu::aboutToShow, this, &AddressableItemContextMenu::aboutToShowSlot);
}

AddressableItemContextMenu::~AddressableItemContextMenu()
{
}

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

void AddressableItemContextMenu::onActionCopyAddress()
{
    auto clipboard = QApplication::clipboard();
    clipboard->setText(RAddressString(offset));
}

void AddressableItemContextMenu::onActionShowXrefs()
{
    emit xrefsTriggered();
    XrefsDialog dialog(mainWindow, nullptr);
    QString tmpName = name;
    if (name.isEmpty()) {
        name = RAddressString(offset);
    }
    dialog.fillRefsForAddress(offset, name, wholeFunction);
    dialog.exec();
}

void AddressableItemContextMenu::onActionAddComment()
{
    CommentsDialog::addOrEditComment(offset, this);
}

void AddressableItemContextMenu::aboutToShowSlot()
{
    if (actionShowInMenu.menu()) {
        actionShowInMenu.menu()->deleteLater();
    }
    actionShowInMenu.setMenu(mainWindow->createShowInMenu(this, offset));
}

void AddressableItemContextMenu::setHasTarget(bool hasTarget)
{
    this->hasTarget = hasTarget;
    for (const auto &action : this->actions()) {
        action->setEnabled(hasTarget);
    }
}

