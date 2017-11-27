#include "DisassemblyContextMenu.h"
#include "dialogs/AsmOptionsDialog.h"
#include "dialogs/CommentsDialog.h"
#include "dialogs/FlagDialog.h"
#include "dialogs/RenameDialog.h"
#include "dialogs/XrefsDialog.h"
#include <QtCore>
#include <QShortcut>

DisassemblyContextMenu::DisassemblyContextMenu(QWidget *parent)
    :   QMenu(parent),
        offset(0),
        actionAddComment(this),
        actionAddFlag(this),
        actionRename(this),
        actionXRefs(this),
        actionDisplayOptions(this)
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
    return {Qt::Key_N};
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
    RAnalFunction *fcn = Core()->functionAt(offset);
    CommentsDialog *c = new CommentsDialog(this);
    if (c->exec())
    {
        QString comment = c->getComment();
        Core()->setComment(offset, comment);
        if (fcn)
        {
            Core()->seek(fcn->addr);
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
    ut64 tgt_addr = UT64_MAX;
    RAnalOp op;
    RCore *core = Core()->core();

    RenameDialog *dialog = new RenameDialog(this);

    r_anal_op(core->anal, &op, offset, core->block + offset - core->offset, 32);
    tgt_addr = op.jump != UT64_MAX ? op.jump : op.ptr;
    if (op.var) {
        RAnalFunction *fcn = r_anal_get_fcn_in (core->anal, offset, 0);
        if (fcn) {
            RAnalVar *bar = r_anal_var_get_byname (core->anal, fcn, op.var->name);
            if (!bar) {
                bar = r_anal_var_get_byname (core->anal, fcn, op.var->name);
                if (!bar) {

                    bar = r_anal_var_get_byname (core->anal, fcn, op.var->name);
                }
            }
            if (bar) {
                dialog->setName(bar->name);
                if (dialog->exec()) {
                    QString new_name = dialog->getName();
                    r_anal_var_rename (core->anal, fcn->addr, bar->scope,
                        bar->kind, bar->name, new_name.toStdString().c_str());
                }
            }
        }
    } else if (tgt_addr != UT64_MAX) {
        RAnalFunction *fcn = r_anal_get_fcn_at (core->anal, tgt_addr, R_ANAL_FCN_TYPE_NULL);
        RFlagItem *f = r_flag_get_i (core->flags, tgt_addr);
        if (fcn) {
            /* Rename function */
            dialog->setName(fcn->name);
            if (dialog->exec()) {
                QString new_name = dialog->getName();
                Core()->renameFunction(fcn->name, new_name);
            }
        } else if (f) {
            /* Rename current flag */
            dialog->setName(f->name);
            if (dialog->exec()) {
                QString new_name = dialog->getName();
                Core()->renameFlag(f->name, new_name);
            }
        } else {
            /* Create new flag */
            dialog->setName("");
            if (dialog->exec()) {
                QString new_name = dialog->getName();
                Core()->addFlag(tgt_addr, new_name, 1);
            }
        }
    }
    r_anal_op_fini (&op);
    emit Core()->commentsChanged();
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
