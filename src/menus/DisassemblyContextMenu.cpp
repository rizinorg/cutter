#include "DisassemblyContextMenu.h"
#include "dialogs/AsmOptionsDialog.h"
#include "dialogs/CommentsDialog.h"
#include "dialogs/FlagDialog.h"
#include "dialogs/RenameDialog.h"
#include "dialogs/XrefsDialog.h"
#include <QtCore>

DisassemblyContextMenu::DisassemblyContextMenu(RVA offset, QWidget *parent) :
    QMenu(parent),
    offset(offset)
{
    actionAddComment.setText("Add comment");
    this->addAction(&actionAddComment);
    actionAddFlag.setText("Add flag");
    this->addAction(&actionAddFlag);
    actionRename.setText("Rename");
    this->addAction(&actionRename);
    actionXRefs.setText("Show xrefs");
    this->addAction(&actionXRefs);
    this->addSeparator();
    actionDisplayOptions.setText("Show options");
    this->addAction(&actionDisplayOptions);

    connect(&actionAddComment, SIGNAL(triggered(bool)), this, SLOT(on_actionAddComment_triggered()));
    connect(&actionAddFlag, SIGNAL(triggered(bool)), this, SLOT(on_actionAddFlag_triggered()));
    connect(&actionRename, SIGNAL(triggered(bool)), this, SLOT(on_actionRename_triggered()));
    connect(&actionXRefs, SIGNAL(triggered(bool)), this, SLOT(on_actionXRefs_triggered()));
    connect(&actionDisplayOptions, SIGNAL(triggered()), this, SLOT(on_actionDisplayOptions_triggered()));
}

DisassemblyContextMenu::~DisassemblyContextMenu()
{
}

void DisassemblyContextMenu::on_actionAddComment_triggered()
{
    RAnalFunction *fcn = CutterCore::getInstance()->functionAt(offset);
    CommentsDialog *c = new CommentsDialog(this);
    if (c->exec())
    {
        QString comment = c->getComment();
        CutterCore::getInstance()->setComment(offset, comment);
        if (fcn)
        {
            CutterCore::getInstance()->seek(fcn->addr);
        }
    }
}

void DisassemblyContextMenu::on_actionAddFlag_triggered()
{
    FlagDialog *dialog = new FlagDialog(offset, this->parentWidget());
    dialog->exec();
}

void DisassemblyContextMenu::on_actionRename_triggered()
{
    // Get function for clicked offset
    RAnalFunction *fcn = CutterCore::getInstance()->functionAt(offset);
    RenameDialog *dialog = new RenameDialog(this);
    // Get function based on click position
    dialog->setFunctionName(fcn->name);
    if (dialog->exec())
    {
        // Get new function name
        QString new_name = dialog->getFunctionName();
        // Rename function in r2 core
        CutterCore::getInstance()->renameFunction(fcn->name, new_name);
        // Seek to new renamed function
        CutterCore::getInstance()->seek(fcn->addr);
    }
}

void DisassemblyContextMenu::on_actionXRefs_triggered()
{
    XrefsDialog *dialog = new XrefsDialog(this);
    dialog->fillRefsForAddress(offset, RAddressString(offset), false);
    dialog->exec();
}

void DisassemblyContextMenu::on_actionDisplayOptions_triggered()
{
    AsmOptionsDialog *dialog = new AsmOptionsDialog(this->parentWidget());
    dialog->show();
}
