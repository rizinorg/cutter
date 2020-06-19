#include "DecompilerContextMenu.h"
#include "dialogs/preferences/PreferencesDialog.h"
#include "MainWindow.h"

#include <QtCore>
#include <QShortcut>
#include <QJsonArray>
#include <QClipboard>
#include <QApplication>
#include <QPushButton>

DecompilerContextMenu::DecompilerContextMenu(QWidget *parent, MainWindow *MainWindow)
    : QMenu(parent)
{

}

DecompilerContextMenu::~DecompilerContextMenu()
{
}

void DecompilerContextMenu::setOffset(RVA offset)
{
    this->offset = offset;

    this->actionSetFunctionVarTypes.setVisible(true);
}

void DecompilerContextMenu::setCanCopy(bool enabled)
{
    this->canCopy = enabled;
}

void DecompilerContextMenu::setCurHighlightedWord(const QString &text)
{
    this->curHighlightedWord = text;
}

void DecompilerContextMenu::aboutToShowSlot()
{
}

QKeySequence DecompilerContextMenu::getCopySequence() const
{
    return QKeySequence::Copy;
}

void DecompilerContextMenu::initAction(QAction *action, QString name, const char *slot)
{
    action->setParent(this);
    parentWidget()->addAction(action);
    action->setText(name);
    if (slot) {
        connect(action, SIGNAL(triggered(bool)), this, slot);
    }
}

void DecompilerContextMenu::initAction(QAction *action, QString name,
                                        const char *slot, QKeySequence keySequence)
{
    initAction(action, name, slot);
    if (keySequence.isEmpty()) {
        return;
    }
    action->setShortcut(keySequence);
    action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
}

void DecompilerContextMenu::initAction(QAction *action, QString name,
                                        const char *slot, QList<QKeySequence> keySequenceList)
{
    initAction(action, name, slot);
    if (keySequenceList.empty()) {
        return;
    }
    action->setShortcuts(keySequenceList);
    action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
}
