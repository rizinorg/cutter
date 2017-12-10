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
        actionRenameUsedHere(this),
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
    actionAddFlag.setShortcut(getAddFlagSequence());

    actionRename.setText(tr("Rename"));
    this->addAction(&actionRename);
    actionRename.setShortcut(getRenameSequence());

    actionRenameUsedHere.setText(("Rename Flag/Fcn/Var Used Here"));
    this->addAction(&actionRenameUsedHere);
    actionRenameUsedHere.setShortcut(getRenameUsedHereSequence());

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
    actionXRefs.setShortcut(getXRefSequence());

    this->addSeparator();
    actionDisplayOptions.setText(tr("Show Options"));
    actionDisplayOptions.setShortcut(getDisplayOptionsSequence());
    this->addAction(&actionDisplayOptions);

    auto pWidget = parentWidget();

#define ADD_SHORTCUT(sequence, slot) { \
    QShortcut *shortcut = new QShortcut((sequence), pWidget); \
    shortcut->setContext(Qt::WidgetWithChildrenShortcut); \
    connect(shortcut, &QShortcut::activated, this, (slot)); \
}
    ADD_SHORTCUT(getCopySequence(), &DisassemblyContextMenu::on_actionCopy_triggered);
    ADD_SHORTCUT(getDisplayOptionsSequence(), &DisassemblyContextMenu::on_actionDisplayOptions_triggered);
    ADD_SHORTCUT(getXRefSequence(), &DisassemblyContextMenu::on_actionXRefs_triggered);
    ADD_SHORTCUT(getCommentSequence(), &DisassemblyContextMenu::on_actionAddComment_triggered);
    ADD_SHORTCUT(getAddFlagSequence(), &DisassemblyContextMenu::on_actionAddFlag_triggered);
    ADD_SHORTCUT(getRenameSequence(), &DisassemblyContextMenu::on_actionRename_triggered);
    ADD_SHORTCUT(getRenameUsedHereSequence(), &DisassemblyContextMenu::on_actionRenameUsedHere_triggered);
#undef ADD_SHORTCUT

    connect(&actionCopy, SIGNAL(triggered(bool)), this, SLOT(on_actionCopy_triggered()));

    connect(&actionAddComment, SIGNAL(triggered(bool)), this, SLOT(on_actionAddComment_triggered()));
    connect(&actionAddFlag, SIGNAL(triggered(bool)), this, SLOT(on_actionAddFlag_triggered()));
    connect(&actionRename, SIGNAL(triggered(bool)), this, SLOT(on_actionRename_triggered()));
    connect(&actionRenameUsedHere, SIGNAL(triggered(bool)), this, SLOT(on_actionRenameUsedHere_triggered()));
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

    QString comment = Core()->cmd("CC." + RAddressString(offset));
    if (comment.isNull() || comment.isEmpty())
    {
        actionAddComment.setText(tr("Add Comment"));
    }
    else
    {
        actionAddComment.setText(tr("Edit Comment"));
    }

    actionCopy.setVisible(canCopy);
    copySeparator->setVisible(canCopy);


    RCore *core = Core()->core();
    RAnalFunction *fcn = r_anal_get_fcn_at (core->anal, offset, R_ANAL_FCN_TYPE_NULL);
    RFlagItem *f = r_flag_get_i (core->flags, offset);
    if (fcn)
    {
        actionRename.setVisible(true);
        actionRename.setText(tr("Rename function \"%1\"").arg(fcn->name));
    }
    else if (f)
    {
        actionRename.setVisible(true);
        actionRename.setText(tr("Rename flag \"%1\"").arg(f->name));
    }
    else
    {
        actionRename.setVisible(false);
    }


    // only show "rename X used here" if there is something to rename
    QJsonArray thingUsedHereArray = Core()->cmdj("anj @ " + QString::number(offset)).array();
    if (!thingUsedHereArray.isEmpty())
    {
        actionRenameUsedHere.setVisible(true);
        QJsonObject thingUsedHere = thingUsedHereArray.first().toObject();
        if (thingUsedHere["type"] == "address")
        {
            RVA offset = thingUsedHere["offset"].toVariant().toULongLong();
            actionRenameUsedHere.setText(tr("Add flag at %1 (used here)").arg(RAddressString(offset)));
        }
        else
        {
            actionRenameUsedHere.setText(tr("Rename \"%1\" (used here)").arg(thingUsedHere["name"].toString()));
        }
    }
    else
    {
        actionRenameUsedHere.setVisible(false);
    }
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

QKeySequence DisassemblyContextMenu::getRenameUsedHereSequence() const
{
    return {Qt::SHIFT + Qt::Key_N};
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
    QString oldComment = Core()->cmd("CC." + RAddressString(offset));
    // Remove newline at the end added by cmd
    oldComment.remove(oldComment.length()-1, 1);
    CommentsDialog *c = new CommentsDialog(this);

    if (oldComment.isNull() || oldComment.isEmpty())
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
    RCore *core = Core()->core();

    RenameDialog *dialog = new RenameDialog(this);

    RAnalFunction *fcn = r_anal_get_fcn_at (core->anal, offset, R_ANAL_FCN_TYPE_NULL);
    RFlagItem *f = r_flag_get_i (core->flags, offset);
    if (fcn)
    {
        /* Rename function */
        dialog->setWindowTitle(tr("Rename function %1").arg(fcn->name));
        dialog->setName(fcn->name);
        if (dialog->exec())
        {
            QString new_name = dialog->getName();
            Core()->renameFunction(fcn->name, new_name);
        }
    }
    else if (f)
    {
        /* Rename current flag */
        dialog->setWindowTitle(tr("Rename flag %1").arg(f->name));
        dialog->setName(f->name);
        if (dialog->exec())
        {
            QString new_name = dialog->getName();
            Core()->renameFlag(f->name, new_name);
        }
    }
    else
    {
        return;
    }
}

void DisassemblyContextMenu::on_actionRenameUsedHere_triggered()
{
    QJsonArray array = Core()->cmdj("anj @ " + QString::number(offset)).array();
    if (array.isEmpty())
    {
        return;
    }

    QJsonObject thingUsedHere = array.first().toObject();
    QString type = thingUsedHere.value("type").toString();

    RenameDialog *dialog = new RenameDialog(this);

    QString oldName;

    if (type == "address")
    {
        RVA offset = thingUsedHere["offset"].toVariant().toULongLong();
        dialog->setWindowTitle(tr("Add flag at %1").arg(RAddressString(offset)));
        dialog->setName("label." + QString::number(offset, 16));
    }
    else
    {
        oldName = thingUsedHere.value("name").toString();
        dialog->setWindowTitle(tr("Rename %1").arg(oldName));
        dialog->setName(oldName);
    }

    if (dialog->exec())
    {
        QString newName = dialog->getName().trimmed();
        if (!newName.isEmpty())
        {
            Core()->cmd("an " + newName + " @ " + QString::number(offset));

            if (type == "address" || type == "flag")
            {
                Core()->triggerFlagsChanged();
            }
            else if (type == "var")
            {
                Core()->triggerVarsChanged();
            }
            else if (type == "function")
            {
                Core()->triggerFunctionRenamed(oldName, newName);
            }
        }
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
