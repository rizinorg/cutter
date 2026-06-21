#include "FlirtContextMenu.h"

#include "MainWindow.h"

#include <QApplication>
#include <QClipboard>
#include <QJsonArray>
#include <QPushButton>
#include <QShortcut>
#include <QtCore>

FlirtContextMenu::FlirtContextMenu(QWidget *parent, MainWindow *mainWindow)
    : QMenu(parent), mainWindow(mainWindow)
{
    actionCopyLine = new QAction(tr("Copy Line"), this);
    actionApplySignature = new QAction(tr("Apply Signature File"), this);

    connect(actionCopyLine, &QAction::triggered, this, &FlirtContextMenu::onActionCopyLine);
    connect(actionApplySignature, &QAction::triggered, this,
            &FlirtContextMenu::onActionApplySignature);

    addAction(actionCopyLine);
    addSeparator();
    addAction(actionApplySignature);

    setHasTarget(false);
}

FlirtContextMenu::~FlirtContextMenu() {}

void FlirtContextMenu::setTarget(const FlirtDescription &flirt)
{
    this->entry = flirt;
    setHasTarget(true);
}

void FlirtContextMenu::clearTarget()
{
    setHasTarget(false);
}

void FlirtContextMenu::onActionCopyLine() const
{
    auto clipboard = QApplication::clipboard();
    const QString text = entry.binName + "\t" + entry.archName + "\t" + entry.archBits + "\t"
            + entry.nModules + "\t" + entry.baseName + "\t" + entry.details;
    clipboard->setText(text);
}

void FlirtContextMenu::onActionApplySignature() const
{
    if (this->hasTarget) {
        Core()->applySignature(entry.filePath);
    }
}

void FlirtContextMenu::setHasTarget(bool hasTarget)
{
    this->hasTarget = hasTarget;
    for (const auto &action : this->actions()) {
        action->setEnabled(hasTarget);
    }
}
