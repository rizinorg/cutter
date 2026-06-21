#include "DisassemblyContextMenu.h"

#include "MainWindow.h"
#include "dialogs/BreakpointsDialog.h"
#include "dialogs/CommentsDialog.h"
#include "dialogs/EditFunctionDialog.h"
#include "dialogs/EditInstructionDialog.h"
#include "dialogs/EditStringDialog.h"
#include "dialogs/EditVariablesDialog.h"
#include "dialogs/FlagDialog.h"
#include "dialogs/GlobalVariableDialog.h"
#include "dialogs/SetToDataDialog.h"
#include "dialogs/XrefsDialog.h"
#include "shortcuts/ShortcutManager.h"

#include <QApplication>
#include <QClipboard>
#include <QInputDialog>
#include <QJsonArray>
#include <QPushButton>
#include <QShortcut>
#include <QtCore>

DisassemblyContextMenu::DisassemblyContextMenu(QWidget *parent, MainWindow *mainWindow)
    : QMenu(parent),
      offset(0),
      canCopy(false),
      mainWindow(mainWindow),
      ioModesController(this),
      actionEditInstruction(this),
      actionNopInstruction(this),
      actionJmpReverse(this),
      actionEditBytes(this),
      actionCopy(this),
      copySeparator(addSeparator()),
      actionCopyAddr(this),
      actionCopyInstrBytes(this),
      actionAddComment(this),
      actionAnalyzeFunction(this),
      actionEditFunction(this),
      actionRename(this),
      actionGlobalVar(this),
      actionSetFunctionVarTypes(this),
      actionXRefs(this),
      actionXRefsForVariables(this),
      actionDeleteComment(this),
      actionDeleteFlag(this),
      actionDeleteFunction(this),
      actionSetBaseBinary(this),
      actionSetBaseOctal(this),
      actionSetBaseDecimal(this),
      actionSetBaseHexadecimal(this),
      actionSetBasePort(this),
      actionSetBaseIPAddr(this),
      actionSetBaseSyscall(this),
      actionSetBaseString(this),
      actionSetBits16(this),
      actionSetBits32(this),
      actionSetBits64(this),
      actionContinueUntil(this),
      actionSetPC(this),
      actionAddBreakpoint(this),
      actionAdvancedBreakpoint(this),
      actionSetToCode(this),
      actionSetAsStringAuto(this),
      actionSetAsStringRemove(this),
      actionSetAsStringAdvanced(this),
      actionSetToDataEx(this),
      actionSetToDataByte(this),
      actionSetToDataWord(this),
      actionSetToDataDword(this),
      actionSetToDataQword(this),
      showInSubmenu(this)
{
    initShortcutAction(&actionCopy, "Disassembly.copy", &DisassemblyContextMenu::copyTriggered);
    addAction(&actionCopy);

    initShortcutAction(&actionCopyAddr, "General.copyAddress",
                       &DisassemblyContextMenu::copyAddrTriggered);
    addAction(&actionCopyAddr);

    initShortcutAction(&actionCopyInstrBytes, "Disassembly.copyInstructionBytes",
                       &DisassemblyContextMenu::copyInstrBytesTriggered);
    addAction(&actionCopyInstrBytes);

    initAction(&showInSubmenu, tr("Show in"), nullptr);
    addAction(&showInSubmenu);

    initShortcutAction(&actionAddComment, "General.addComment",
                       &DisassemblyContextMenu::addCommentTriggered);
    addAction(&actionAddComment);

    initShortcutAction(&actionSetFunctionVarTypes, "Disassembly.retypeLocals",
                       &DisassemblyContextMenu::setFunctionVarTypesTriggered);
    addAction(&actionSetFunctionVarTypes);

    initShortcutAction(&actionEditFunction, "Disassembly.editFunction",
                       &DisassemblyContextMenu::editFunctionTriggered);
    addAction(&actionEditFunction);

    initAction(&actionDeleteComment, tr("Delete comment"),
               &DisassemblyContextMenu::deleteCommentTriggered);
    addAction(&actionDeleteComment);

    initAction(&actionDeleteFlag, tr("Delete flag"), &DisassemblyContextMenu::deleteFlagTriggered);
    addAction(&actionDeleteFlag);

    initShortcutAction(&actionDeleteFunction, "Disassembly.undefineFunction",
                       &DisassemblyContextMenu::deleteFunctionTriggered);
    addAction(&actionDeleteFunction);

    initShortcutAction(&actionAnalyzeFunction, "Disassembly.defineFunction",
                       &DisassemblyContextMenu::analyzeFunctionTriggered);
    addAction(&actionAnalyzeFunction);

    addSeparator();

    addAddAtMenu();

    addSetBaseMenu();

    addSetBitsMenu();

    structureOffsetMenu = addMenu(tr("Structure offset"));
    connect(structureOffsetMenu, &QMenu::triggered, this,
            &DisassemblyContextMenu::structureOffsetMenuTriggered);

    addSetAsMenu();

    addSeparator();

    initShortcutAction(&actionXRefs, "General.showXRefs", &DisassemblyContextMenu::xRefsTriggered);
    addAction(&actionXRefs);

    initShortcutAction(&actionXRefsForVariables, "Disassembly.XRefsForVariables",
                       &DisassemblyContextMenu::xRefsForVariablesTriggered);
    addAction(&actionXRefsForVariables);

    addSeparator();

    addEditMenu();

    addSeparator();

    addBreakpointMenu();
    addDebugMenu();

    addSeparator();

    if (mainWindow) {
        pluginMenu = mainWindow->getContextMenuExtensions(MainWindow::ContextMenuType::Disassembly);
        pluginActionMenuAction = addMenu(pluginMenu);
    }

    addSeparator();

    connect(this, &DisassemblyContextMenu::aboutToShow, this,
            &DisassemblyContextMenu::aboutToShowSlot);
    connect(this, &DisassemblyContextMenu::aboutToHide, this,
            &DisassemblyContextMenu::aboutToHideSlot);
}

DisassemblyContextMenu::~DisassemblyContextMenu() {}

QWidget *DisassemblyContextMenu::parentForDialog()
{
    return parentWidget();
}

void DisassemblyContextMenu::addAddAtMenu()
{
    setAsMenu = addMenu(tr("Add at..."));

    initShortcutAction(&actionRename, "Disassembly.rename",
                       &DisassemblyContextMenu::renameTriggered);
    setAsMenu->addAction(&actionRename);

    initShortcutAction(&actionGlobalVar, "Disassembly.globalVariable",
                       &DisassemblyContextMenu::globalVarTriggered);
    setAsMenu->addAction(&actionGlobalVar);
}

void DisassemblyContextMenu::addSetBaseMenu()
{
    setBaseMenu = addMenu(tr("Set base of immediate value to.."));

    initAction(&actionSetBaseBinary, tr("Binary"), nullptr);
    setBaseMenu->addAction(&actionSetBaseBinary);
    connect(&actionSetBaseBinary, &QAction::triggered, this, [this] { setBase("b"); });

    initAction(&actionSetBaseOctal, tr("Octal"), nullptr);
    setBaseMenu->addAction(&actionSetBaseOctal);
    connect(&actionSetBaseOctal, &QAction::triggered, this, [this] { setBase("o"); });

    initAction(&actionSetBaseDecimal, tr("Decimal"), nullptr);
    setBaseMenu->addAction(&actionSetBaseDecimal);
    connect(&actionSetBaseDecimal, &QAction::triggered, this, [this] { setBase("d"); });

    initAction(&actionSetBaseHexadecimal, tr("Hexadecimal"), nullptr);
    setBaseMenu->addAction(&actionSetBaseHexadecimal);
    connect(&actionSetBaseHexadecimal, &QAction::triggered, this, [this] { setBase("h"); });

    initAction(&actionSetBasePort, tr("Network Port"), nullptr);
    setBaseMenu->addAction(&actionSetBasePort);
    connect(&actionSetBasePort, &QAction::triggered, this, [this] { setBase("p"); });

    initAction(&actionSetBaseIPAddr, tr("IP Address"), nullptr);
    setBaseMenu->addAction(&actionSetBaseIPAddr);
    connect(&actionSetBaseIPAddr, &QAction::triggered, this, [this] { setBase("i"); });

    initAction(&actionSetBaseSyscall, tr("Syscall"), nullptr);
    setBaseMenu->addAction(&actionSetBaseSyscall);
    connect(&actionSetBaseSyscall, &QAction::triggered, this, [this] { setBase("S"); });

    initAction(&actionSetBaseString, tr("String"), nullptr);
    setBaseMenu->addAction(&actionSetBaseString);
    connect(&actionSetBaseString, &QAction::triggered, this, [this] { setBase("s"); });
}

void DisassemblyContextMenu::addSetBitsMenu()
{
    setBitsMenu = addMenu(tr("Set current bits to..."));

    initAction(&actionSetBits16, "16", nullptr);
    setBitsMenu->addAction(&actionSetBits16);
    connect(&actionSetBits16, &QAction::triggered, this, [this] { setBits(16); });

    initAction(&actionSetBits32, "32", nullptr);
    setBitsMenu->addAction(&actionSetBits32);
    connect(&actionSetBits32, &QAction::triggered, this, [this] { setBits(32); });

    initAction(&actionSetBits64, "64", nullptr);
    setBitsMenu->addAction(&actionSetBits64);
    connect(&actionSetBits64, &QAction::triggered, this, [this] { setBits(64); });
}

void DisassemblyContextMenu::addSetAsMenu()
{
    setAsMenu = addMenu(tr("Set as..."));

    initShortcutAction(&actionSetToCode, "Disassembly.setToCode",
                       &DisassemblyContextMenu::setToCodeTriggered);
    setAsMenu->addAction(&actionSetToCode);

    setAsString = setAsMenu->addMenu(tr("String..."));

    initShortcutAction(&actionSetAsStringAuto, "Disassembly.setAsString",
                       &DisassemblyContextMenu::setAsStringTriggered);
    initAction(&actionSetAsStringRemove, tr("Remove"),
               &DisassemblyContextMenu::setAsStringRemoveTriggered);
    initShortcutAction(&actionSetAsStringAdvanced, "Disassembly.setAsStringAdvanced",
                       &DisassemblyContextMenu::setAsStringAdvancedTriggered);

    setAsString->addAction(&actionSetAsStringAuto);
    setAsString->addAction(&actionSetAsStringRemove);
    setAsString->addAction(&actionSetAsStringAdvanced);

    addSetToDataMenu();
}

void DisassemblyContextMenu::addSetToDataMenu()
{
    setToDataMenu = setAsMenu->addMenu(tr("Data..."));

    initAction(&actionSetToDataByte, tr("Byte"), nullptr);
    setToDataMenu->addAction(&actionSetToDataByte);
    connect(&actionSetToDataByte, &QAction::triggered, this, [this] { setToData(1); });

    initAction(&actionSetToDataWord, tr("Word"), nullptr);
    setToDataMenu->addAction(&actionSetToDataWord);
    connect(&actionSetToDataWord, &QAction::triggered, this, [this] { setToData(2); });

    initAction(&actionSetToDataDword, tr("Dword"), nullptr);
    setToDataMenu->addAction(&actionSetToDataDword);
    connect(&actionSetToDataDword, &QAction::triggered, this, [this] { setToData(4); });

    initAction(&actionSetToDataQword, tr("Qword"), nullptr);
    setToDataMenu->addAction(&actionSetToDataQword);
    connect(&actionSetToDataQword, &QAction::triggered, this, [this] { setToData(8); });

    initShortcutAction(&actionSetToDataEx, "Disassembly.setToDataEx",
                       &DisassemblyContextMenu::setToDataExTriggered);
    actionSetToDataEx.setText(tr("Advanced"));
    setToDataMenu->addAction(&actionSetToDataEx);

    auto switchAction = new QAction(this);
    initShortcutAction(switchAction, "Disassembly.setToData",
                       &DisassemblyContextMenu::setToDataTriggered);
    setToDataMenu->addAction(switchAction);
}

void DisassemblyContextMenu::addEditMenu()
{
    editMenu = addMenu(tr("Edit"));

    initAction(&actionEditInstruction, tr("Instruction"),
               &DisassemblyContextMenu::editInstructionTriggered);
    editMenu->addAction(&actionEditInstruction);

    initAction(&actionNopInstruction, tr("Nop Instruction"),
               &DisassemblyContextMenu::nopInstructionTriggered);
    editMenu->addAction(&actionNopInstruction);

    initAction(&actionEditBytes, tr("Bytes"), &DisassemblyContextMenu::editBytesTriggered);
    editMenu->addAction(&actionEditBytes);

    initAction(&actionJmpReverse, tr("Reverse Jump"), &DisassemblyContextMenu::jmpReverseTriggered);
    editMenu->addAction(&actionJmpReverse);
}

void DisassemblyContextMenu::addBreakpointMenu()
{
    breakpointMenu = addMenu(tr("Breakpoint"));

    initShortcutAction(&actionAddBreakpoint, "Debug.toggleBreakpoint",
                       &DisassemblyContextMenu::addBreakpointTriggered);
    breakpointMenu->addAction(&actionAddBreakpoint);
    initShortcutAction(&actionAdvancedBreakpoint, "Debug.advancedBreakpoint",
                       &DisassemblyContextMenu::advancedBreakpointTriggered);
    breakpointMenu->addAction(&actionAdvancedBreakpoint);
}

void DisassemblyContextMenu::addDebugMenu()
{
    debugMenu = addMenu(tr("Debug"));

    initAction(&actionContinueUntil, tr("Continue until line"),
               &DisassemblyContextMenu::continueUntilTriggered);
    debugMenu->addAction(&actionContinueUntil);

    initAction(&actionSetPC, "Set PC", &DisassemblyContextMenu::setPCTriggered);
    debugMenu->addAction(&actionSetPC);
}

QVector<DisassemblyContextMenu::ThingUsedHere> DisassemblyContextMenu::getThingUsedHere(RVA offset)
{
    RzCoreLocked core(Core());
    auto p = fromOwned(rz_core_analysis_name(core, offset), rz_core_analysis_name_free);
    if (!p) {
        return {};
    }

    QVector<ThingUsedHere> result;
    ThingUsedHere th;
    th.offset = p->offset;
    th.name = Config()->getConfigBool("asm.flags.real") && p->realname ? p->realname : p->name;
    switch (p->type) {
    case RZ_CORE_ANALYSIS_NAME_TYPE_FLAG:
        th.type = ThingUsedHere::Type::Flag;
        break;
    case RZ_CORE_ANALYSIS_NAME_TYPE_FUNCTION:
        th.type = ThingUsedHere::Type::Function;
        break;
    case RZ_CORE_ANALYSIS_NAME_TYPE_VAR:
        th.type = ThingUsedHere::Type::Var;
        break;
    case RZ_CORE_ANALYSIS_NAME_TYPE_ADDRESS:
    default:
        th.type = ThingUsedHere::Type::Address;
        break;
    }
    result.push_back(th);
    return result;
}

void DisassemblyContextMenu::setOffset(RVA offset)
{
    this->offset = offset;
    this->actionSetFunctionVarTypes.setVisible(true);
}

void DisassemblyContextMenu::setCanCopy(bool enabled)
{
    this->canCopy = enabled;
}

void DisassemblyContextMenu::setCurHighlightedWord(const QString &text)
{
    this->curHighlightedWord = text;
    // Update the renaming options only when a new word is selected
    setupRenaming();
}

DisassemblyContextMenu::ThingUsedHere DisassemblyContextMenu::getThingAt(ut64 address)
{
    ThingUsedHere tuh;
    auto core = Core()->lock();
    const RzAnalysisFunction *fcn = Core()->functionAt(address);
    const RzFlagItem *flag = rz_flag_get_i(core->flags, address);

    // We will lookup through existing rizin types to find something relevant

    if (fcn != nullptr) {
        // It is a function
        tuh.type = ThingUsedHere::Type::Function;
        tuh.name = fcn->name;
    } else if (flag != nullptr) {
        // It is a flag
        tuh.type = ThingUsedHere::Type::Flag;
        if (Config()->getConfigBool("asm.flags.real") && flag->realname) {
            tuh.name = flag->realname;
        } else {
            tuh.name = flag->name;
        }
    } else {
        // Consider it an address
        tuh.type = ThingUsedHere::Type::Address;
    }

    tuh.offset = address;
    return tuh;
}

void DisassemblyContextMenu::buildRenameMenu(ThingUsedHere *tuh)
{
    if (!tuh) {
        qWarning() << "Unexpected behavior null pointer passed to "
                      "DisassemblyContextMenu::buildRenameMenu";
        doRenameAction = RENAME_DO_NOTHING;
        return;
    }

    actionDeleteFlag.setVisible(false);
    if (tuh->type == ThingUsedHere::Type::Address) {
        doRenameAction = RENAME_ADD_FLAG;
        doRenameInfo.name = rzAddressString(tuh->offset);
        doRenameInfo.addr = tuh->offset;
        actionRename.setText(tr("Add flag at %1 (used here)").arg(doRenameInfo.name));
    } else if (tuh->type == ThingUsedHere::Type::Function) {
        doRenameAction = RENAME_FUNCTION;
        doRenameInfo.name = tuh->name;
        doRenameInfo.addr = tuh->offset;
        actionRename.setText(tr("Rename \"%1\"").arg(doRenameInfo.name));
    } else if (tuh->type == ThingUsedHere::Type::Var) {
        doRenameAction = RENAME_LOCAL;
        doRenameInfo.name = tuh->name;
        doRenameInfo.addr = tuh->offset;
        actionRename.setText(tr("Rename local \"%1\"").arg(tuh->name));
    } else if (tuh->type == ThingUsedHere::Type::Flag) {
        doRenameAction = RENAME_FLAG;
        doRenameInfo.name = tuh->name;
        doRenameInfo.addr = tuh->offset;
        actionRename.setText(tr("Rename flag \"%1\" (used here)").arg(doRenameInfo.name));
        actionDeleteFlag.setVisible(true);
    } else {
        qWarning() << "Unexpected renaming type";
        doRenameAction = RENAME_DO_NOTHING;
    }
}

void DisassemblyContextMenu::setupRenaming()
{
    // We parse our highlighted word as an address
    const ut64 selection = Core()->num(curHighlightedWord);

    // First, let's try to see if current line (offset) contains a local variable or a function
    ThingUsedHere *tuh = nullptr;
    ThingUsedHere thingAt;
    auto things = getThingUsedHere(offset);
    for (auto &thing : things) {
        if (thing.offset == selection || thing.name == curHighlightedWord) {
            // We matched something on current line
            tuh = &thing;
            break;
        }
    }

    if (!tuh) {
        // Nothing matched on current line, is there anything valid coming from our selection?
        thingAt = getThingAt(selection);

        if (thingAt.offset == 0) {
            // We parsed something which resolved to 0, it's very likely nothing interesting was
            // selected So we fallback on current line offset
            thingAt = getThingAt(offset);
        }

        // However, since for the moment selection selects *every* lines which match a specific
        // offset, make sure we didn't want to select a local variable rather than the function
        // itself
        if (thingAt.type == ThingUsedHere::Type::Function) {
            auto vars = Core()->getVariables(offset);
            for (const auto &v : std::as_const(vars)) {
                if (v.name == curHighlightedWord) {
                    // This is a local variable
                    thingAt.type = ThingUsedHere::Type::Var;
                    thingAt.name = v.name;
                    break;
                }
            }
        }

        // In any case, thingAt will contain something we can rename
        tuh = &thingAt;
    }

    // Now, build the renaming menu and show it
    buildRenameMenu(tuh);

    auto name = rzAddressString(tuh->offset);
    actionGlobalVar.setText(tr("Add or change global variable at %1 (used here)").arg(name));

    actionRename.setVisible(true);
    actionGlobalVar.setVisible(true);
}

void DisassemblyContextMenu::aboutToShowSlot()
{
    // check if set immediate base menu makes sense
    auto cdb = Core()->getRzCoreDecodedBytesSingle(offset);

    const bool immBase = cdb && (cdb->an_op.val || cdb->an_op.ptr);
    setBaseMenu->menuAction()->setVisible(immBase);
    setBitsMenu->menuAction()->setVisible(true);

    // Create structure offset menu if it makes sense
    QString memBaseReg; // Base register
    st64 memDisp = 0; // Displacement

    if (cdb) {
        const CutterJson operands =
                Core()->parseJson("opex", rz_structured_data_to_json(cdb->an_op.opex), nullptr);

        // Loop through both the operands of the instruction
        for (const auto &operand : operands) {
            if (operand["type"].toString() == "mem" && !operand["base"].toString().contains("bp")
                && operand["disp"].toSt64() > 0) {

                // The current operand is the one which has an immediate displacement
                memBaseReg = operand["base"].toString();
                memDisp = operand["disp"].toSt64();
                break;
            }
        }
    }

    if (memBaseReg.isEmpty()) {
        // hide structure offset menu
        structureOffsetMenu->menuAction()->setVisible(false);
    } else {
        // show structure offset menu
        structureOffsetMenu->menuAction()->setVisible(true);
        structureOffsetMenu->clear();

        RzCoreLocked core(Core());
        const RzTypeDB *typedb = rz_analysis_get_type_db(core->analysis);
        RzList *typeoffs = rz_type_db_get_by_offset(typedb, memDisp);
        if (typeoffs) {
            for (const auto &ty : CutterRzList<RzTypePath>(typeoffs)) {
                if (RZ_STR_ISEMPTY(ty->path)) {
                    continue;
                }
                structureOffsetMenu->addAction("[" + memBaseReg + " + " + ty->path + "]")
                        ->setData(QString(ty->path));
            }
            rz_list_free(typeoffs);
        }

        if (structureOffsetMenu->isEmpty()) {
            // No possible offset was found so hide the menu
            structureOffsetMenu->menuAction()->setVisible(false);
        }
    }

    actionAnalyzeFunction.setVisible(true);

    // Show the option to remove a defined string only if a string is defined in this address
    const QString stringDefinition = Core()->getMetaString(offset);
    actionSetAsStringRemove.setVisible(!stringDefinition.isEmpty());

    const QString comment = Core()->getCommentAt(offset);

    if (comment.isNull() || comment.isEmpty()) {
        actionDeleteComment.setVisible(false);
        actionAddComment.setText(tr("Add Comment"));
    } else {
        actionDeleteComment.setVisible(true);
        actionAddComment.setText(tr("Edit Comment"));
    }

    actionCopy.setVisible(canCopy);
    copySeparator->setVisible(canCopy);

    // Handle renaming of variable, function, flag, ...
    // Note: This might be useless if we consider setCurrentHighlightedWord is always called before
    setupRenaming();

    // Only show retype for local vars if in a function
    const RzAnalysisFunction *inFcn = Core()->functionIn(offset);
    if (inFcn) {
        auto vars = Core()->getVariables(offset);
        actionSetFunctionVarTypes.setVisible(!vars.empty());
        actionEditFunction.setVisible(true);
        actionEditFunction.setText(tr("Edit function \"%1\"").arg(inFcn->name));
    } else {
        actionSetFunctionVarTypes.setVisible(false);
        actionEditFunction.setVisible(false);
    }

    // Decide to show Reverse jmp option
    showReverseJmpQuery();

    if (showInSubmenu.menu() != nullptr) {
        showInSubmenu.menu()->deleteLater();
    }
    showInSubmenu.setMenu(mainWindow->createShowInMenu(this, offset));

    // Only show debug options if we are currently debugging
    debugMenu->menuAction()->setVisible(Core()->currentlyDebugging);
    const bool hasBreakpoint = Core()->breakpointIndexAt(offset) > -1;
    actionAddBreakpoint.setText(hasBreakpoint ? tr("Remove breakpoint") : tr("Add breakpoint"));
    actionAdvancedBreakpoint.setText(hasBreakpoint ? tr("Edit breakpoint")
                                                   : tr("Advanced breakpoint"));
    const QString progCounterName = Core()->getRegisterName("PC").toUpper();
    actionSetPC.setText("Set " + progCounterName + " here");

    if (pluginMenu) {
        pluginActionMenuAction->setVisible(!pluginMenu->isEmpty());
        for (QAction *pluginAction : pluginMenu->actions()) {
            pluginAction->setData(QVariant::fromValue(offset));
        }
    }

    const bool isLocalVar = isHighlightedWordLocalVar();
    actionXRefsForVariables.setVisible(isLocalVar);
    if (isLocalVar) {
        actionXRefsForVariables.setText(tr("X-Refs for %1").arg(curHighlightedWord));
    }

    actionEditInstruction.setEnabled(Core()->hasAssembler());
}

void DisassemblyContextMenu::aboutToHideSlot()
{
    actionXRefsForVariables.setVisible(true);
}

void DisassemblyContextMenu::editInstructionTriggered()
{
    if (!ioModesController.prepareForWriting()) {
        return;
    }
    EditInstructionDialog e(EDIT_TEXT, parentForDialog());
    e.setWindowTitle(tr("Edit Instruction at %1").arg(rzAddressString(offset)));

    const QString oldInstructionOpcode = Core()->getInstructionOpcode(offset);
    const QString oldInstructionBytes = Core()->getInstructionBytes(offset);

    e.setInstruction(oldInstructionOpcode);

    if (e.exec()) {
        const bool fillWithNops = e.needsNops();
        const QString userInstructionOpcode = e.getInstruction();
        if (userInstructionOpcode != oldInstructionOpcode) {
            Core()->editInstruction(offset, userInstructionOpcode, fillWithNops);
        }
    }
}

void DisassemblyContextMenu::nopInstructionTriggered()
{
    if (!ioModesController.prepareForWriting()) {
        return;
    }
    Core()->nopInstruction(offset);
}

void DisassemblyContextMenu::showReverseJmpQuery()
{
    actionJmpReverse.setVisible(false);
    auto cdb = Core()->getRzCoreDecodedBytesSingle(offset);
    if (!cdb) {
        return;
    }
    if (cdb->an_op.type == RZ_ANALYSIS_OP_TYPE_CJMP) {
        actionJmpReverse.setVisible(true);
    }
}

void DisassemblyContextMenu::jmpReverseTriggered()
{
    if (!ioModesController.prepareForWriting()) {
        return;
    }
    Core()->jmpReverse(offset);
}

void DisassemblyContextMenu::editBytesTriggered()
{
    if (!ioModesController.prepareForWriting()) {
        return;
    }
    EditInstructionDialog e(EDIT_BYTES, parentForDialog());
    e.setWindowTitle(tr("Edit Bytes at %1").arg(rzAddressString(offset)));

    const QString oldBytes = Core()->getInstructionBytes(offset);
    e.setInstruction(oldBytes);

    if (e.exec()) {
        const QString bytes = e.getInstruction();
        if (bytes != oldBytes) {
            Core()->editBytes(offset, bytes);
        }
    }
}

void DisassemblyContextMenu::copyTriggered()
{
    emit copy();
}

void DisassemblyContextMenu::copyAddrTriggered() const
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(rzAddressString(offset));
}

void DisassemblyContextMenu::copyInstrBytesTriggered() const
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(Core()->getInstructionBytes(offset));
}

void DisassemblyContextMenu::addBreakpointTriggered() const
{
    Core()->toggleBreakpoint(offset);
}

void DisassemblyContextMenu::advancedBreakpointTriggered()
{
    const int index = Core()->breakpointIndexAt(offset);
    if (index >= 0) {
        BreakpointsDialog::editBreakpoint(Core()->getBreakpointAt(offset), parentForDialog());
    } else {
        BreakpointsDialog::createNewBreakpoint(offset, parentForDialog());
    }
}

void DisassemblyContextMenu::continueUntilTriggered() const
{
    Core()->continueUntilDebug(offset);
}

void DisassemblyContextMenu::setPCTriggered() const
{
    const QString progCounterName = Core()->getRegisterName("PC");
    Core()->setRegister(progCounterName, rzAddressString(offset).toUpper());
}

void DisassemblyContextMenu::addCommentTriggered()
{
    CommentsDialog::addOrEditComment(offset, parentForDialog());
}

void DisassemblyContextMenu::analyzeFunctionTriggered()
{
    RVA flagOffset;
    QString name = Core()->nearestFlag(offset, &flagOffset);
    if (name.isEmpty() || flagOffset != offset) {
        // Create a default name for the function
        QString pfx = Config()->getConfigString("analysis.fcnprefix");
        if (pfx.isEmpty()) {
            pfx = QString("fcn");
        }
        name = pfx + QString::asprintf(".%llx", static_cast<unsigned long long>(offset));
    }

    // Create dialog
    QInputDialog inputDialog(parentForDialog());
    inputDialog.resize(500, 100);
    inputDialog.setWindowTitle(tr("New function at %1").arg(rzAddressString(offset)));
    inputDialog.setLabelText(tr("Function name:"));
    inputDialog.setTextValue(name);
    inputDialog.setWindowFlags(Qt::Window | Qt::WindowMinimizeButtonHint);

    if (inputDialog.exec() != QDialog::Accepted) {
        return;
    }

    const QString functionName = inputDialog.textValue().trimmed();

    if (!functionName.isEmpty()) {
        Core()->createFunctionAt(offset, functionName);
    }
}

void DisassemblyContextMenu::renameTriggered()
{
    bool ok = false;
    if (doRenameAction == RENAME_FUNCTION) {
        const QString newName = QInputDialog::getText(
                this->mainWindow, tr("Rename function %2").arg(doRenameInfo.name),
                tr("Function name:"), QLineEdit::Normal, doRenameInfo.name, &ok);
        if (ok && !newName.isEmpty()) {
            Core()->renameFunction(doRenameInfo.addr, newName);
        }
    } else if (doRenameAction == RENAME_FLAG || doRenameAction == RENAME_ADD_FLAG) {
        FlagDialog dialog(doRenameInfo.addr, parentForDialog());
        ok = dialog.exec();
    } else if (doRenameAction == RENAME_LOCAL) {
        const RzAnalysisFunction *fcn = Core()->functionIn(offset);
        if (fcn) {
            EditVariablesDialog dialog(fcn->addr, curHighlightedWord, parentForDialog());
            if (!dialog.empty()) {
                // Don't show the dialog if there are no variables
                ok = dialog.exec();
            }
        }
    } else if (doRenameAction == RENAME_DO_NOTHING) {
        // Do nothing
    } else {
        qWarning() << "Unhandled renaming action: " << doRenameAction;
        assert(false);
    }

    if (ok) {
        // Rebuild menu in case the user presses the rename shortcut directly before clicking
        setupRenaming();
    }
}

void DisassemblyContextMenu::globalVarTriggered()
{
    bool ok = false;
    GlobalVariableDialog dialog(doRenameInfo.addr, parentForDialog());
    ok = dialog.exec();

    if (ok) {
        // Rebuild menu in case the user presses the rename shortcut directly before clicking
        setupRenaming();
    }
}

void DisassemblyContextMenu::setFunctionVarTypesTriggered()
{
    const RzAnalysisFunction *fcn = Core()->functionIn(offset);

    if (!fcn) {
        QMessageBox::critical(this, tr("Re-type Local Variables"),
                              tr("You must be in a function to define variable types."));
        return;
    }

    EditVariablesDialog dialog(fcn->addr, curHighlightedWord, parentForDialog());
    if (dialog.empty()) { // don't show the dialog if there are no variables
        return;
    }
    dialog.exec();
}

void DisassemblyContextMenu::xRefsTriggered()
{
    XrefsDialog dialog(mainWindow);
    dialog.fillRefsForAddress(offset, rzAddressString(offset), false);
    dialog.exec();
}

void DisassemblyContextMenu::xRefsForVariablesTriggered()
{
    if (isHighlightedWordLocalVar()) {
        XrefsDialog dialog(mainWindow);
        dialog.fillRefsForVariable(curHighlightedWord, offset);
        dialog.exec();
    }
}

void DisassemblyContextMenu::setToCodeTriggered() const
{
    Core()->setToCode(offset);
}

void DisassemblyContextMenu::setAsStringTriggered() const
{
    Core()->setAsString(offset);
}

void DisassemblyContextMenu::setAsStringRemoveTriggered() const
{
    Core()->removeString(offset);
}

void DisassemblyContextMenu::setAsStringAdvancedTriggered()
{
    EditStringDialog dialog(parentForDialog());
    const int predictedStrSize = Core()->getString(offset).size();
    dialog.setStringSizeValue(predictedStrSize);
    dialog.setStringStartAddress(offset);

    if (!dialog.exec()) {
        return;
    }

    uint64_t strAddr = 0U;
    if (!dialog.getStringStartAddress(strAddr)) {
        QMessageBox::critical(this->window(), tr("Wrong address"),
                              tr("Can't edit string at this address"));
        return;
    }
    CutterCore::StringTypeFormats coreStringType = CutterCore::StringTypeFormats::None;

    const auto strSize = dialog.getStringSizeValue();
    const auto strType = dialog.getStringType();
    switch (strType) {
    case EditStringDialog::StringType::Auto:
        coreStringType = CutterCore::StringTypeFormats::None;
        break;
    case EditStringDialog::StringType::ASCII_LATIN1:
        coreStringType = CutterCore::StringTypeFormats::ASCII_LATIN1;
        break;
    case EditStringDialog::StringType::UTF8:
        coreStringType = CutterCore::StringTypeFormats::UTF8;
        break;
    };

    Core()->setAsString(strAddr, strSize, coreStringType);
}

void DisassemblyContextMenu::setToDataTriggered()
{
    int size = Core()->sizeofDataMeta(offset);
    if (size > 8 || (size && (size & (size - 1)))) {
        return;
    }
    if (size == 0 || size == 8) {
        size = 1;
    } else {
        size *= 2;
    }
    setToData(size);
}

void DisassemblyContextMenu::setToDataExTriggered()
{
    SetToDataDialog dialog(offset, parentForDialog());
    if (!dialog.exec()) {
        return;
    }
    setToData(dialog.getItemSize(), dialog.getItemCount());
}

void DisassemblyContextMenu::structureOffsetMenuTriggered(QAction *action) const
{
    Core()->applyStructureOffset(action->data().toString(), offset);
}

void DisassemblyContextMenu::deleteCommentTriggered() const
{
    Core()->delComment(offset);
}

void DisassemblyContextMenu::deleteFlagTriggered() const
{
    Core()->delFlag(offset);
}

void DisassemblyContextMenu::deleteFunctionTriggered() const
{
    Core()->delFunction(offset);
}

void DisassemblyContextMenu::editFunctionTriggered()
{
    auto core = Core()->lock();
    EditFunctionDialog dialog(parentForDialog());
    RzAnalysisFunction *fcn = rz_analysis_get_fcn_in(core->analysis, offset, 0);

    if (fcn) {
        dialog.setWindowTitle(tr("Edit function %1").arg(fcn->name));
        dialog.setNameText(fcn->name);

        const QString startAddrText = "0x" + QString::number(fcn->addr, 16);
        dialog.setStartAddrText(startAddrText);

        dialog.setStackSizeText(QString::number(fcn->stack));

        QStringList callConList;
        RzList *list = rz_analysis_calling_conventions(core->analysis);
        if (!list) {
            return;
        }
        RzListIter *iter;
        const char *cc;
        CutterRzListForeach (list, iter, const char, cc) {
            callConList << cc;
        }
        rz_list_free(list);

        dialog.setCallConList(callConList);
        dialog.setCallConSelected(fcn->cc);

        if (dialog.exec()) {
            const QString newName = dialog.getNameText();
            rz_core_analysis_function_rename(core, fcn->addr, newName.toStdString().c_str());
            const QString newStartAddr = dialog.getStartAddrText();
            fcn->addr = Core()->math(newStartAddr);
            const QString newStackSize = dialog.getStackSizeText();
            fcn->stack = int(Core()->math(newStackSize));

            const QByteArray newCC = dialog.getCallConSelected().toUtf8();
            if (!newCC.isEmpty()) {
                rz_analysis_function_set_cc(core->analysis, fcn, newCC.constData());
            }

            emit Core()->functionsChanged();
        }
    }
}

void DisassemblyContextMenu::setBase(const QString &base) const
{
    Core()->setImmediateBase(base, offset);
}

void DisassemblyContextMenu::setBits(int bits) const
{
    Core()->setCurrentBits(bits, offset);
}

void DisassemblyContextMenu::setToData(int size, int repeat) const
{
    Core()->setToData(offset, size, repeat);
}

template<typename SlotFunc>
QAction *DisassemblyContextMenu::addAnonymousAction(QString name, SlotFunc slot,
                                                    QKeySequence keySequence)
{
    auto action = new QAction(this);
    addAction(action);
    anonymousActions.append(action);
    initAction(action, name, slot, keySequence);
    return action;
}

template<typename SlotFunc>
void DisassemblyContextMenu::initAction(QAction *action, const QString &name, SlotFunc slot)
{
    action->setParent(this);
    parentWidget()->addAction(action);
    action->setText(name);

    if constexpr (!std::is_same_v<SlotFunc, std::nullptr_t>) {
        if (slot != nullptr) {
            connect(action, &QAction::triggered, this, slot);
        }
    }
}

template<typename SlotFunc>
void DisassemblyContextMenu::initAction(QAction *action, QString name, SlotFunc slot,
                                        const QKeySequence &keySequence)
{
    initAction(action, name, slot);
    if (keySequence.isEmpty()) {
        return;
    }
    action->setShortcut(keySequence);
    action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
}

template<typename SlotFunc>
void DisassemblyContextMenu::initShortcutAction(QAction *action, const QString &id, SlotFunc slot)
{
    Shortcuts()->setupAction(*action, id);
    action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    if (slot != nullptr) {
        connect(action, &QAction::triggered, this, slot);
    }
}

bool DisassemblyContextMenu::isHighlightedWordLocalVar()
{
    const QList<VariableDescription> variables = Core()->getVariables(offset);
    for (const VariableDescription &var : variables) {
        if (var.name == curHighlightedWord) {
            return true;
        }
    }
    return false;
}
