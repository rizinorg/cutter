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
        canCopy(false),
        actionCopy(this),
        actionAddComment(this),
        actionAddFlag(this),
        actionRename(this),
        actionXRefs(this),
        actionDisplayOptions(this),
        actionSetBaseBinary(this),
        actionSetBaseOctal(this),
        actionSetBaseDecimal(this),
        actionSetBaseHexadecimal(this),
        actionSetBasePort(this),
        actionSetBaseIPAddr(this),
        actionSetBaseSyscall(this),
        actionSetBaseString(this)
{
    actionCopy.setText(tr("Copy"));
    this->addAction(&actionCopy);
    actionCopy.setShortcut(getCopySequence());
    copySeparator = addSeparator();

    actionAddComment.setText(tr("Add Comment"));
    this->addAction(&actionAddComment);
    actionAddComment.setShortcut(getCommentSequence());

    actionAddFlag.setText(tr("Add Flag"));
    this->addAction(&actionAddFlag);
    actionAddComment.setShortcut(getAddFlagSequence());

    actionRename.setText(tr("Rename"));
    this->addAction(&actionRename);
    actionAddComment.setShortcut(getRenameSequence());

    setBaseMenu = new QMenu(tr("Set Immediate Base to..."), this);
    setBaseMenuAction = addMenu(setBaseMenu);
    actionSetBaseBinary.setText(tr("Binary"));
    setBaseMenu->addAction(&actionSetBaseBinary);
    actionSetBaseOctal.setText(tr("Octal"));
    setBaseMenu->addAction(&actionSetBaseOctal);
    actionSetBaseDecimal.setText(tr("Decimal"));
    setBaseMenu->addAction(&actionSetBaseDecimal);
    actionSetBaseHexadecimal.setText(tr("Hexadecimal"));
    setBaseMenu->addAction(&actionSetBaseHexadecimal);
    actionSetBasePort.setText(tr("Network Port"));
    setBaseMenu->addAction(&actionSetBasePort);
    actionSetBaseIPAddr.setText(tr("IP Address"));
    setBaseMenu->addAction(&actionSetBaseIPAddr);
    actionSetBaseSyscall.setText(tr("Syscall"));
    setBaseMenu->addAction(&actionSetBaseSyscall);
    actionSetBaseString.setText(tr("String"));
    setBaseMenu->addAction(&actionSetBaseString);

    this->addSeparator();
    actionXRefs.setText(tr("Show X-Refs"));
    this->addAction(&actionXRefs);
    actionAddComment.setShortcut(getXRefSequence());

    this->addSeparator();
    actionDisplayOptions.setText(tr("Show Options"));
    actionAddComment.setShortcut(getDisplayOptionsSequence());
    this->addAction(&actionDisplayOptions);

    auto pWidget = parentWidget();

    QShortcut *shortcut_Copy = new QShortcut(getCopySequence(), pWidget);
    shortcut_Copy->setContext(Qt::WidgetWithChildrenShortcut);
    connect(shortcut_Copy, &QShortcut::activated,
            this, &DisassemblyContextMenu::on_actionCopy_triggered);

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

    connect(&actionCopy, SIGNAL(triggered(bool)), this, SLOT(on_actionCopy_triggered()));

    connect(&actionAddComment, SIGNAL(triggered(bool)), this, SLOT(on_actionAddComment_triggered()));
    connect(&actionAddFlag, SIGNAL(triggered(bool)), this, SLOT(on_actionAddFlag_triggered()));
    connect(&actionRename, SIGNAL(triggered(bool)), this, SLOT(on_actionRename_triggered()));
    connect(&actionXRefs, SIGNAL(triggered(bool)), this, SLOT(on_actionXRefs_triggered()));
    connect(&actionDisplayOptions, SIGNAL(triggered()), this, SLOT(on_actionDisplayOptions_triggered()));

    connect(&actionSetBaseBinary, SIGNAL(triggered(bool)), this, SLOT(on_actionSetBaseBinary_triggered()));
    connect(&actionSetBaseOctal, SIGNAL(triggered(bool)), this, SLOT(on_actionSetBaseOctal_triggered()));
    connect(&actionSetBaseDecimal, SIGNAL(triggered(bool)), this, SLOT(on_actionSetBaseDecimal_triggered()));
    connect(&actionSetBaseHexadecimal, SIGNAL(triggered(bool)), this, SLOT(on_actionSetBaseHexadecimal_triggered()));
    connect(&actionSetBasePort, SIGNAL(triggered(bool)), this, SLOT(on_actionSetBasePort_triggered()));
    connect(&actionSetBaseIPAddr, SIGNAL(triggered(bool)), this, SLOT(on_actionSetBaseIPAddr_triggered()));
    connect(&actionSetBaseSyscall, SIGNAL(triggered(bool)), this, SLOT(on_actionSetBaseSyscall_triggered()));
    connect(&actionSetBaseString, SIGNAL(triggered(bool)), this, SLOT(on_actionSetBaseString_triggered()));

    connect(this, SIGNAL(aboutToShow()), this, SLOT(aboutToShowSlot()));
}

void DisassemblyContextMenu::setOffset(RVA offset)
{
    this->offset = offset;
}

void DisassemblyContextMenu::setCanCopy(bool enabled)
{
    this->canCopy = enabled;
}

void DisassemblyContextMenu::aboutToShowSlot()
{
    // check if set immediate base menu makes sense
    QJsonObject instObject = Core()->cmdj("aoj @ " + QString::number(offset)).array().first().toObject();
    auto keys = instObject.keys();
    bool immBase = keys.contains("val") || keys.contains("ptr");
    setBaseMenuAction->setVisible(immBase);

    QJsonObject disasObject = Core()->cmdj("pdj 1 @ " + QString::number(offset)).array().first().toObject();
    QString comment = disasObject["comment"].toString();
    if (comment.isNull() || QByteArray::fromBase64(comment.toUtf8()).isEmpty())
    {
        actionAddComment.setText(tr("Add Comment"));
    }
    else
    {
        actionAddComment.setText(tr("Edit Comment"));
    }

    actionCopy.setVisible(canCopy);
    copySeparator->setVisible(canCopy);
}

QKeySequence DisassemblyContextMenu::getCopySequence() const
{
    return QKeySequence::Copy;
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

void DisassemblyContextMenu::on_actionCopy_triggered()
{
    emit copy();
}

void DisassemblyContextMenu::on_actionAddComment_triggered()
{
    QJsonObject disasObject = Core()->cmdj("pdj 1 @ " + QString::number(offset)).array().first().toObject();
    QString oldComment = QString::fromUtf8(QByteArray::fromBase64(disasObject["comment"].toString().toUtf8()));

    CommentsDialog *c = new CommentsDialog(this);

    if (oldComment.isNull() || QByteArray::fromBase64(oldComment.toUtf8()).isEmpty())
    {
        c->setWindowTitle(tr("Add Comment at %1").arg(RAddressString(offset)));
    }
    else
    {
        c->setWindowTitle(tr("Edit Comment at %1").arg(RAddressString(offset)));
    }

    c->setComment(oldComment);
    if (c->exec())
    {
        QString comment = c->getComment();
        if (comment.isEmpty())
        {
            Core()->delComment(offset);
        }
        else
        {
            Core()->setComment(offset, comment);
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

void DisassemblyContextMenu::on_actionSetBaseBinary_triggered()
{
    Core()->setImmediateBase("b", offset);
}

void DisassemblyContextMenu::on_actionSetBaseOctal_triggered()
{
    Core()->setImmediateBase("o", offset);
}

void DisassemblyContextMenu::on_actionSetBaseDecimal_triggered()
{
    Core()->setImmediateBase("d", offset);
}

void DisassemblyContextMenu::on_actionSetBaseHexadecimal_triggered()
{
    Core()->setImmediateBase("h", offset);
}

void DisassemblyContextMenu::on_actionSetBasePort_triggered()
{
    Core()->setImmediateBase("p", offset);
}

void DisassemblyContextMenu::on_actionSetBaseIPAddr_triggered()
{
    Core()->setImmediateBase("i", offset);
}

void DisassemblyContextMenu::on_actionSetBaseSyscall_triggered()
{
    Core()->setImmediateBase("S", offset);
}

void DisassemblyContextMenu::on_actionSetBaseString_triggered()
{
    Core()->setImmediateBase("s", offset);
}
