#include "DisassemblyContextMenu.h"
#include "dialogs/AsmOptionsDialog.h"
#include "dialogs/CommentsDialog.h"
#include "dialogs/FlagDialog.h"
#include "dialogs/RenameDialog.h"
#include "dialogs/XrefsDialog.h"
#include <QtCore>
#include <QShortcut>

DisassemblyContextMenu::DisassemblyContextMenu(RVA offset, QWidget *parent) :
    QMenu(parent),
    offset(offset)
{
    init();
}

DisassemblyContextMenu::DisassemblyContextMenu(QWidget*parent)
    :   QMenu(parent)
{
    init();
}

void DisassemblyContextMenu::setOffset(RVA offset)
{
    this->offset = offset;
}

QKeySequence DisassemblyContextMenu::getCommentSequence() const
{
    return {";"};
}

QKeySequence DisassemblyContextMenu::getAddFlagSequence() const
{
    return {}; //TODO insert correct sequence
}

QKeySequence DisassemblyContextMenu::getRenameSequence() const
{
    return {}; //TODO insert correct sequence
}

QKeySequence DisassemblyContextMenu::getXRefSequence() const
{
    return {Qt::Key_X};
}

QKeySequence DisassemblyContextMenu::getDisplayOptionsSequence() const
{
    return {}; //TODO insert correct sequence
}

void DisassemblyContextMenu::init()
{
    actionAddComment.setText("Add comment");
    this->addAction(&actionAddComment);
    actionAddComment.setShortcut(getCommentSequence());

    actionAddFlag.setText("Add flag");
    this->addAction(&actionAddFlag);
    actionAddComment.setShortcut(getAddFlagSequence());

    actionRename.setText("Rename");
    this->addAction(&actionRename);
    actionAddComment.setShortcut(getRenameSequence());

    actionXRefs.setText("Show xrefs");
    this->addAction(&actionXRefs);
    actionAddComment.setShortcut(getXRefSequence());

    this->addSeparator();
    actionDisplayOptions.setText("Show options");
    actionAddComment.setShortcut(getDisplayOptionsSequence());
    this->addAction(&actionDisplayOptions);

    auto pWidget = parentWidget();

    QShortcut *shortcut_dispOptions = new QShortcut(getDisplayOptionsSequence(), pWidget);
    shortcut_dispOptions->setContext(Qt::WidgetWithChildrenShortcut);
    connect(shortcut_dispOptions, &QShortcut::activated,
            this, &DisassemblyContextMenu::on_actionDisplayOptions_triggered);

    QShortcut *shortcut_x = new QShortcut(getXRefSequence(), pWidget);
    shortcut_x->setContext(Qt::WidgetWithChildrenShortcut);
    connect(shortcut_x, &QShortcut::activated,
            this, &DisassemblyContextMenu::on_actionXRefs_triggered);

    QShortcut *shortcut_comment = new QShortcut(getCommentSequence(), pWidget);
    shortcut_comment->setContext(Qt::WidgetWithChildrenShortcut);
    connect(shortcut_comment, &QShortcut::activated,
            this, &DisassemblyContextMenu::on_actionAddComment_triggered);

    QShortcut *shortcut_addFlag = new QShortcut(getAddFlagSequence(), pWidget);
    shortcut_addFlag->setContext(Qt::WidgetWithChildrenShortcut);
    connect(shortcut_addFlag, &QShortcut::activated,
            this, &DisassemblyContextMenu::on_actionAddFlag_triggered);

    QShortcut *shortcut_renameSequence = new QShortcut(getRenameSequence(), pWidget);
    shortcut_renameSequence->setContext(Qt::WidgetWithChildrenShortcut);
    connect(shortcut_renameSequence, &QShortcut::activated,
            this, &DisassemblyContextMenu::on_actionRename_triggered);

    connect(&actionAddComment, SIGNAL(triggered(bool)), this, SLOT(on_actionAddComment_triggered()));
    connect(&actionAddFlag, SIGNAL(triggered(bool)), this, SLOT(on_actionAddFlag_triggered()));
    connect(&actionRename, SIGNAL(triggered(bool)), this, SLOT(on_actionRename_triggered()));
    connect(&actionXRefs, SIGNAL(triggered(bool)), this, SLOT(on_actionXRefs_triggered()));
    connect(&actionDisplayOptions, SIGNAL(triggered()), this, SLOT(on_actionDisplayOptions_triggered()));
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
