#include "DecompilerContextMenu.h"

#include "MainWindow.h"
#include "dialogs/BreakpointsDialog.h"
#include "dialogs/CommentsDialog.h"
#include "dialogs/EditVariablesDialog.h"
#include "dialogs/XrefsDialog.h"
#include "shortcuts/ShortcutManager.h"

#include <QApplication>
#include <QClipboard>
#include <QInputDialog>
#include <QJsonArray>
#include <QPushButton>
#include <QShortcut>
#include <QtCore>

#include <utility>

DecompilerContextMenu::DecompilerContextMenu(QWidget *parent, MainWindow *mainWindow)
    : QMenu(parent),
      mainWindow(mainWindow),
      curHighlightedWord(QString()),
      offset(0),
      decompiledFunctionAddress(RVA_INVALID),
      isTogglingBreakpoints(false),
      annotationHere(nullptr),
      actionCopy(this),
      actionCopyInstructionAddress(tr("Copy instruction address (<address>)"), this),
      actionCopyReferenceAddress(this),
      copySeparator(addSeparator()),
      actionShowInSubmenu(tr("Show in"), this),
      actionAddComment(this),
      actionDeleteComment(tr("Delete comment"), this),
      actionRenameThingHere(this),
      actionDeleteName(tr("Delete <name>"), this),
      actionEditFunctionVariables(this),
      actionXRefs(this),
      actionToggleBreakpoint(this),
      actionAdvancedBreakpoint(this),
      breakpointsInLineMenu(new QMenu(this)),
      actionContinueUntil(tr("Continue until line"), this),
      actionSetPC(tr("Set PC"), this)
{
    setActionCopy(); // Sets all three copy actions
    addSeparator();

    setActionShowInSubmenu();

    setActionAddComment();
    setActionDeleteComment();

    setActionRenameThingHere();
    setActionDeleteName();

    setActionXRefs();

    setActionEditFunctionVariables();

    addSeparator();
    addBreakpointMenu();
    addDebugMenu();

    setShortcutContextInActions(this);

    connect(this, &DecompilerContextMenu::aboutToShow, this,
            &DecompilerContextMenu::aboutToShowSlot);
    connect(this, &DecompilerContextMenu::aboutToHide, this,
            &DecompilerContextMenu::aboutToHideSlot);
}

DecompilerContextMenu::~DecompilerContextMenu() {}

QWidget *DecompilerContextMenu::parentForDialog()
{
    return parentWidget();
}

void DecompilerContextMenu::setAnnotationHere(RzCodeAnnotation *annotation)
{
    annotationHere = annotation;
}

void DecompilerContextMenu::setCurHighlightedWord(QString word)
{
    curHighlightedWord = std::move(word);
}

void DecompilerContextMenu::setOffset(RVA newOffset)
{
    offset = newOffset;
}

void DecompilerContextMenu::setDecompiledFunctionAddress(RVA functionAddr)
{
    decompiledFunctionAddress = functionAddr;
}

void DecompilerContextMenu::setFirstOffsetInLine(RVA firstOffset)
{
    firstOffsetInLine = firstOffset;
}

RVA DecompilerContextMenu::getFirstOffsetInLine() const
{
    return firstOffsetInLine;
}

void DecompilerContextMenu::setAvailableBreakpoints(QVector<RVA> offsetList)
{
    availableBreakpoints = std::move(offsetList);
}

void DecompilerContextMenu::setupBreakpointsInLineMenu()
{
    breakpointsInLineMenu->clear();
    for (auto curOffset : std::as_const(this->availableBreakpoints)) {
        const QAction *action = breakpointsInLineMenu->addAction(rzAddressString(curOffset));
        connect(action, &QAction::triggered, this, [this, curOffset] {
            BreakpointsDialog::editBreakpoint(Core()->getBreakpointAt(curOffset), this);
        });
    }
}

void DecompilerContextMenu::setShortcutContextInActions(QMenu *menu)
{
    for (QAction *action : menu->actions()) {
        if (action->isSeparator()) {
            // Do nothing
        } else if (action->menu()) {
            setShortcutContextInActions(action->menu());
        } else {
            action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        }
    }
}

void DecompilerContextMenu::setIsTogglingBreakpoints(bool isToggling)
{
    isTogglingBreakpoints = isToggling;
}

bool DecompilerContextMenu::getIsTogglingBreakpoints() const
{
    return isTogglingBreakpoints;
}

void DecompilerContextMenu::aboutToHideSlot()
{
    actionAddComment.setVisible(true);
    actionRenameThingHere.setVisible(true);
    actionRenameThingHere.setEnabled(true);
    actionDeleteName.setVisible(false);
    actionEditFunctionVariables.setVisible(true);
    actionEditFunctionVariables.setEnabled(true);
    actionXRefs.setVisible(true);
    setToolTipsVisible(false);
}

void DecompilerContextMenu::aboutToShowSlot()
{
    if (this->firstOffsetInLine != RVA_MAX) {
        actionShowInSubmenu.setVisible(true);
        const QString comment = Core()->getCommentAt(firstOffsetInLine);
        actionAddComment.setVisible(true);
        if (comment.isEmpty()) {
            actionDeleteComment.setVisible(false);
            actionAddComment.setText(tr("Add Comment"));
        } else {
            actionDeleteComment.setVisible(true);
            actionAddComment.setText(tr("Edit Comment"));
        }
    } else {
        actionShowInSubmenu.setVisible(false);
        actionAddComment.setVisible(false);
        actionDeleteComment.setVisible(false);
    }

    setupBreakpointsInLineMenu();

    // Only show debug options if we are currently debugging
    debugMenu->menuAction()->setVisible(Core()->currentlyDebugging);

    const bool hasBreakpoint = !this->availableBreakpoints.isEmpty();
    const int numberOfBreakpoints = this->availableBreakpoints.size();
    if (numberOfBreakpoints == 0) {
        actionToggleBreakpoint.setText(tr("Add breakpoint"));
    } else if (numberOfBreakpoints == 1) {
        actionToggleBreakpoint.setText(tr("Remove breakpoint"));
    } else {
        actionToggleBreakpoint.setText(tr("Remove all breakpoints in line"));
    }
    if (numberOfBreakpoints > 1) {
        actionAdvancedBreakpoint.setMenu(breakpointsInLineMenu);
    } else {
        actionAdvancedBreakpoint.setMenu(nullptr);
    }
    actionAdvancedBreakpoint.setText(hasBreakpoint ? tr("Edit breakpoint")
                                                   : tr("Advanced breakpoint"));

    const QString progCounterName = Core()->getRegisterName("PC").toUpper();
    actionSetPC.setText(tr("Set %1 here").arg(progCounterName));

    if (!annotationHere
        || annotationHere->type
                == RZ_CODE_ANNOTATION_TYPE_CONSTANT_VARIABLE) { // If constant, don't show rename
                                                                // and targeted show-in
        actionRenameThingHere.setVisible(false);
        copySeparator->setVisible(false);
    } else {
        copySeparator->setVisible(true);
        if (annotationHere->type == RZ_CODE_ANNOTATION_TYPE_FUNCTION_NAME) {
            actionRenameThingHere.setText(
                    tr("Rename function %1").arg(QString(annotationHere->reference.name)));
        } else if (annotationHere->type == RZ_CODE_ANNOTATION_TYPE_GLOBAL_VARIABLE) {
            RzCoreLocked core = Core()->lock();
            const RzFlagItem *flagDetails =
                    rz_flag_get_i(core->flags, annotationHere->reference.offset);
            if (flagDetails) {
                actionRenameThingHere.setText(tr("Rename %1").arg(QString(flagDetails->name)));
                actionDeleteName.setText(tr("Remove %1").arg(QString(flagDetails->name)));
                actionDeleteName.setVisible(true);
            } else {
                actionRenameThingHere.setText(tr("Add name to %1").arg(curHighlightedWord));
            }
        }
    }
    actionCopyInstructionAddress.setText(
            tr("Copy instruction address (%1)").arg(rzAddressString(offset)));
    if (isReference()) {
        actionCopyReferenceAddress.setVisible(true);
        const RVA referenceAddr = annotationHere->reference.offset;
        RzCoreLocked core = Core()->lock();
        const RzFlagItem *flagDetails = rz_flag_get_i(core->flags, referenceAddr);
        if (annotationHere->type == RZ_CODE_ANNOTATION_TYPE_FUNCTION_NAME) {
            actionCopyReferenceAddress.setText(tr("Copy address of %1 (%2)")
                                                       .arg(QString(annotationHere->reference.name),
                                                            rzAddressString(referenceAddr)));
        } else if (flagDetails) {
            actionCopyReferenceAddress.setText(
                    tr("Copy address of %1 (%2)")
                            .arg(flagDetails->name, rzAddressString(referenceAddr)));
        } else {
            actionCopyReferenceAddress.setText(
                    tr("Copy address (%1)").arg(rzAddressString(referenceAddr)));
        }
    } else {
        actionXRefs.setVisible(false);
        actionCopyReferenceAddress.setVisible(false);
    }
    if (actionShowInSubmenu.menu() != nullptr) {
        actionShowInSubmenu.menu()->deleteLater();
    }
    actionShowInSubmenu.setMenu(mainWindow->createShowInMenu(this, offset));
    updateTargetMenuActions();

    if (!isFunctionVariable()) {
        actionEditFunctionVariables.setVisible(false);
    } else {
        actionEditFunctionVariables.setText(
                tr("Edit variable %1").arg(QString(annotationHere->variable.name)));
        actionRenameThingHere.setText(
                tr("Rename variable %1").arg(QString(annotationHere->variable.name)));
        if (!variablePresentInRizin()) {
            actionEditFunctionVariables.setDisabled(true);
            actionRenameThingHere.setDisabled(true);
            setToolTipsVisible(true);
        }
    }
}

// Set up actions

void DecompilerContextMenu::setActionCopy() // Set all three copy actions
{
    Shortcuts()->setupAction(actionCopy, "Decompiler.copy");
    connect(&actionCopy, &QAction::triggered, this, &DecompilerContextMenu::actionCopyTriggered);
    addAction(&actionCopy);
    connect(&actionCopyInstructionAddress, &QAction::triggered, this,
            &DecompilerContextMenu::actionCopyInstructionAddressTriggered);
    addAction(&actionCopyInstructionAddress);

    Shortcuts()->setupAction(actionCopyReferenceAddress, "Decompiler.copyReferenceAddress");
    connect(&actionCopyReferenceAddress, &QAction::triggered, this,
            &DecompilerContextMenu::actionCopyReferenceAddressTriggered);
    addAction(&actionCopyReferenceAddress);
}

void DecompilerContextMenu::setActionShowInSubmenu()
{
    addAction(&actionShowInSubmenu);
}

void DecompilerContextMenu::setActionAddComment()
{
    Shortcuts()->setupAction(actionAddComment, "General.addComment");
    connect(&actionAddComment, &QAction::triggered, this,
            &DecompilerContextMenu::actionAddCommentTriggered);
    addAction(&actionAddComment);
}

void DecompilerContextMenu::setActionDeleteComment()
{
    connect(&actionDeleteComment, &QAction::triggered, this,
            &DecompilerContextMenu::actionDeleteCommentTriggered);
    addAction(&actionDeleteComment);
}

void DecompilerContextMenu::setActionXRefs()
{
    Shortcuts()->setupAction(actionXRefs, "General.showXRefs");
    connect(&actionXRefs, &QAction::triggered, this, &DecompilerContextMenu::actionXRefsTriggered);
    addAction(&actionXRefs);
}

void DecompilerContextMenu::setActionRenameThingHere()
{
    Shortcuts()->setupAction(actionRenameThingHere, "Decompiler.renameThingHere");
    connect(&actionRenameThingHere, &QAction::triggered, this,
            &DecompilerContextMenu::actionRenameThingHereTriggered);
    addAction(&actionRenameThingHere);
    actionRenameThingHere.setToolTip(
            tr("Can't rename this variable.<br>"
               "Only local variables defined in disassembly can be renamed."));
}

void DecompilerContextMenu::setActionDeleteName()
{
    connect(&actionDeleteName, &QAction::triggered, this,
            &DecompilerContextMenu::actionDeleteNameTriggered);
    addAction(&actionDeleteName);
    actionDeleteName.setVisible(false);
}

void DecompilerContextMenu::setActionEditFunctionVariables()
{
    Shortcuts()->setupAction(actionEditFunctionVariables, "Decompiler.editFunctionVariables");
    connect(&actionEditFunctionVariables, &QAction::triggered, this,
            &DecompilerContextMenu::actionEditFunctionVariablesTriggered);
    addAction(&actionEditFunctionVariables);
    actionEditFunctionVariables.setToolTip(
            tr("Can't edit this variable.<br>"
               "Only local variables defined in disassembly can be edited."));
}

void DecompilerContextMenu::setActionToggleBreakpoint()
{
    Shortcuts()->setupAction(actionToggleBreakpoint, "Debug.toggleBreakpoint");
    connect(&actionToggleBreakpoint, &QAction::triggered, this,
            &DecompilerContextMenu::actionToggleBreakpointTriggered);
}

void DecompilerContextMenu::setActionAdvancedBreakpoint()
{
    Shortcuts()->setupAction(actionAdvancedBreakpoint, "Debug.advancedBreakpoint");
    connect(&actionAdvancedBreakpoint, &QAction::triggered, this,
            &DecompilerContextMenu::actionAdvancedBreakpointTriggered);
}

void DecompilerContextMenu::setActionContinueUntil()
{
    connect(&actionContinueUntil, &QAction::triggered, this,
            &DecompilerContextMenu::actionContinueUntilTriggered);
}

void DecompilerContextMenu::setActionSetPC()
{
    connect(&actionSetPC, &QAction::triggered, this, &DecompilerContextMenu::actionSetPCTriggered);
}

// Set up action responses

void DecompilerContextMenu::actionCopyTriggered()
{
    emit copy();
}

void DecompilerContextMenu::actionCopyInstructionAddressTriggered() const
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(rzAddressString(offset));
}

void DecompilerContextMenu::actionCopyReferenceAddressTriggered()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(rzAddressString(annotationHere->reference.offset));
}

void DecompilerContextMenu::actionAddCommentTriggered()
{
    CommentsDialog::addOrEditComment(this->firstOffsetInLine, parentForDialog());
}

void DecompilerContextMenu::actionDeleteCommentTriggered() const
{
    Core()->delComment(this->firstOffsetInLine);
}

void DecompilerContextMenu::actionRenameThingHereTriggered()
{
    if (!annotationHere || annotationHere->type == RZ_CODE_ANNOTATION_TYPE_CONSTANT_VARIABLE) {
        return;
    }
    RzCoreLocked core = Core()->lock();
    bool ok;
    auto type = annotationHere->type;
    if (type == RZ_CODE_ANNOTATION_TYPE_FUNCTION_NAME) {
        const QString currentName(annotationHere->reference.name);
        const RVA funcAddr = annotationHere->reference.offset;
        const RzAnalysisFunction *func = Core()->functionAt(funcAddr);
        if (func == nullptr) {
            const QString functionName = QInputDialog::getText(
                    parentForDialog(),
                    tr("Define this function at %2").arg(rzAddressString(funcAddr)),
                    tr("Function name:"), QLineEdit::Normal, currentName, &ok);
            if (ok && !functionName.isEmpty()) {
                Core()->createFunctionAt(funcAddr, functionName);
            }
        } else {
            const QString newName = QInputDialog::getText(
                    parentForDialog(), tr("Rename function %2").arg(currentName),
                    tr("Function name:"), QLineEdit::Normal, currentName, &ok);
            if (ok && !newName.isEmpty()) {
                Core()->renameFunction(funcAddr, newName);
            }
        }
    } else if (type == RZ_CODE_ANNOTATION_TYPE_GLOBAL_VARIABLE) {
        const RVA varAddr = annotationHere->reference.offset;
        const RzFlagItem *flagDetails = rz_flag_get_i(core->flags, varAddr);
        if (flagDetails) {
            const QString newName = QInputDialog::getText(
                    parentForDialog(), tr("Rename %2").arg(flagDetails->name), tr("Enter name"),
                    QLineEdit::Normal, flagDetails->name, &ok);
            if (ok && !newName.isEmpty()) {
                Core()->renameFlag(flagDetails->name, newName);
            }
        } else {
            const QString newName = QInputDialog::getText(
                    parentForDialog(), tr("Add name to %2").arg(curHighlightedWord),
                    tr("Enter name"), QLineEdit::Normal, curHighlightedWord, &ok);
            if (ok && !newName.isEmpty()) {
                Core()->addFlag(varAddr, newName, 1);
            }
        }
    } else if (isFunctionVariable()) {
        if (!variablePresentInRizin()) {
            // Show can't rename this variable dialog
            QMessageBox::critical(
                    parentForDialog(),
                    tr("Rename local variable %1").arg(QString(annotationHere->variable.name)),
                    tr("Can't rename this variable. "
                       "Only local variables defined in disassembly can be renamed."));
            return;
        }
        const QString oldName(annotationHere->variable.name);
        const QString newName =
                QInputDialog::getText(parentForDialog(), tr("Rename %2").arg(oldName),
                                      tr("Enter name"), QLineEdit::Normal, oldName, &ok);
        if (ok && !newName.isEmpty()) {
            Core()->renameFunctionVariable(newName, oldName, decompiledFunctionAddress);
        }
    }
}

void DecompilerContextMenu::actionDeleteNameTriggered()
{
    Core()->delFlag(annotationHere->reference.offset);
}

void DecompilerContextMenu::actionEditFunctionVariablesTriggered()
{
    if (!isFunctionVariable()) {
        return;
    } else if (!variablePresentInRizin()) {
        QMessageBox::critical(
                parentForDialog(),
                tr("Edit local variable %1").arg(QString(annotationHere->variable.name)),
                tr("Can't edit this variable. "
                   "Only local variables defined in disassembly can be edited."));
        return;
    }
    EditVariablesDialog dialog(decompiledFunctionAddress, QString(annotationHere->variable.name),
                               parentForDialog());
    dialog.exec();
}

void DecompilerContextMenu::actionXRefsTriggered()
{
    if (!isReference()) {
        return;
    }
    XrefsDialog dialog(mainWindow);
    const QString displayString = (annotationHere->type == RZ_CODE_ANNOTATION_TYPE_FUNCTION_NAME)
            ? QString(annotationHere->reference.name)
            : rzAddressString(annotationHere->reference.offset);
    dialog.fillRefsForAddress(annotationHere->reference.offset, displayString, false);
    dialog.exec();
}

void DecompilerContextMenu::actionToggleBreakpointTriggered()
{
    if (!this->availableBreakpoints.isEmpty()) {
        setIsTogglingBreakpoints(true);
        for (auto offsetToRemove : std::as_const(this->availableBreakpoints)) {
            Core()->toggleBreakpoint(offsetToRemove);
        }
        this->availableBreakpoints.clear();
        setIsTogglingBreakpoints(false);
        return;
    }
    if (this->firstOffsetInLine == RVA_MAX) {
        return;
    }

    Core()->toggleBreakpoint(this->firstOffsetInLine);
}

void DecompilerContextMenu::actionAdvancedBreakpointTriggered()
{
    if (!availableBreakpoints.empty()) {
        // Edit the earliest breakpoint in the line
        BreakpointsDialog::editBreakpoint(
                Core()->getBreakpointAt(this->availableBreakpoints.first()), this);
    } else {
        // Add a breakpoint to the earliest offset in the line
        BreakpointsDialog::createNewBreakpoint(this->firstOffsetInLine, this);
    }
}

void DecompilerContextMenu::actionContinueUntilTriggered() const
{
    Core()->continueUntilDebug(offset);
}

void DecompilerContextMenu::actionSetPCTriggered() const
{
    const QString progCounterName = Core()->getRegisterName("PC");
    Core()->setRegister(progCounterName, rzAddressString(offset).toUpper());
}

// Set up menus

void DecompilerContextMenu::addBreakpointMenu()
{
    breakpointMenu = addMenu(tr("Breakpoint"));

    setActionToggleBreakpoint();
    breakpointMenu->addAction(&actionToggleBreakpoint);
    setActionAdvancedBreakpoint();
    breakpointMenu->addAction(&actionAdvancedBreakpoint);
}

void DecompilerContextMenu::addDebugMenu()
{
    debugMenu = addMenu(tr("Debug"));

    setActionContinueUntil();
    debugMenu->addAction(&actionContinueUntil);
    setActionSetPC();
    debugMenu->addAction(&actionSetPC);
}

void DecompilerContextMenu::updateTargetMenuActions()
{
    for (auto action : std::as_const(showTargetMenuActions)) {
        removeAction(action);
        auto menu = action->menu();
        if (menu) {
            menu->deleteLater();
        }
        action->deleteLater();
    }
    showTargetMenuActions.clear();
    RzCoreLocked core = Core()->lock();
    if (isReference()) {
        QString name;
        QMenu *menu = nullptr;
        if (annotationHere->type == RZ_CODE_ANNOTATION_TYPE_GLOBAL_VARIABLE
            || annotationHere->type == RZ_CODE_ANNOTATION_TYPE_CONSTANT_VARIABLE) {
            menu = mainWindow->createShowInMenu(this, annotationHere->reference.offset,
                                                AddressTypeHint::Data);
            const RVA varAddr = annotationHere->reference.offset;
            const RzFlagItem *flagDetails = rz_flag_get_i(core->flags, varAddr);
            if (flagDetails) {
                name = tr("Show %1 in").arg(flagDetails->name);
            } else {
                name = tr("Show %1 in").arg(rzAddressString(annotationHere->reference.offset));
            }
        } else if (annotationHere->type == RZ_CODE_ANNOTATION_TYPE_FUNCTION_NAME) {
            menu = mainWindow->createShowInMenu(this, annotationHere->reference.offset,
                                                AddressTypeHint::Function);
            name = tr("%1 (%2)").arg(QString(annotationHere->reference.name),
                                     rzAddressString(annotationHere->reference.offset));
        }
        auto action = new QAction(name, this);
        showTargetMenuActions.append(action);
        action->setMenu(menu);
        insertActions(copySeparator, showTargetMenuActions);
    }
}

bool DecompilerContextMenu::isReference()
{
    return (annotationHere && rz_annotation_is_reference(annotationHere));
}

bool DecompilerContextMenu::isFunctionVariable()
{
    return (annotationHere && rz_annotation_is_variable(annotationHere));
}

bool DecompilerContextMenu::variablePresentInRizin()
{
    const QString variableName(annotationHere->variable.name);
    const QList<VariableDescription> variables = Core()->getVariables(offset);
    for (const VariableDescription &var : variables) {
        if (var.name == variableName) {
            return true;
        }
    }
    return false;
}
