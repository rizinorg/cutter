#include "FlirtContextMenu.h"
#include "MainWindow.h"

#include <QtCore>
#include <QShortcut>
#include <QJsonArray>
#include <QClipboard>
#include <QApplication>
#include <QPushButton>

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

void FlirtContextMenu::onActionCopyLine()
{
    auto clipboard = QApplication::clipboard();
    QString text = entry.bin_name + "\t" + entry.arch_name + "\t" + entry.arch_bits + "\t"
            + entry.n_modules + "\t" + entry.base_name + "\t" + entry.details;
    clipboard->setText(text);
}

void FlirtContextMenu::onActionApplySignature()
{
    if (this->hasTarget) {
        Core()->applySignature(entry.file_path);
    }
}

void FlirtContextMenu::setHasTarget(bool hasTarget)
{
    this->hasTarget = hasTarget;
    for (const auto &action : this->actions()) {
        action->setEnabled(hasTarget);
    }
}
