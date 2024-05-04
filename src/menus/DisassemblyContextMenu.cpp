#include "DisassemblyContextMenu.h"
#include "common/DisassemblyPreview.h"
#include "dialogs/preferences/PreferencesDialog.h"
#include "dialogs/EditInstructionDialog.h"
#include "dialogs/CommentsDialog.h"
#include "dialogs/FlagDialog.h"
#include "dialogs/GlobalVariableDialog.h"
#include "dialogs/XrefsDialog.h"
#include "dialogs/EditVariablesDialog.h"
#include "dialogs/SetToDataDialog.h"
#include "dialogs/EditFunctionDialog.h"
#include "dialogs/EditStringDialog.h"
#include "dialogs/BreakpointsDialog.h"
#include "MainWindow.h"

#include <QtCore>
#include <QShortcut>
#include <QJsonArray>
#include <QDebug>
#include <QTextEdit>
#include <QClipboard>
#include <QApplication>
#include <QPushButton>
#include <QInputDialog>

DisassemblyContextMenu::DisassemblyContextMenu(QWidget *parent, MainWindow *mainWindow)
    : QMenu(parent),
      offset(0),
      canCopy(false),
      mainWindow(mainWindow),
      actionEditInstruction(this),
      actionNopInstruction(this),
      actionJmpReverse(this),
      actionEditBytes(this),
      actionCopy(this),
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
      actionDisplayOptions(this),
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
    initAction(&actionCopy, tr("Copy"), SLOT(on_actionCopy_triggered()), getCopySequence());
    addAction(&actionCopy);

    initAction(&actionCopyAddr, tr("Copy address"), SLOT(on_actionCopyAddr_triggered()),
               getCopyAddressSequence());
    addAction(&actionCopyAddr);

    initAction(&actionCopyInstrBytes, tr("Copy instruction bytes"),
               SLOT(on_actionCopyInstrBytes_triggered()), getCopyInstrBytesSequence());
    addAction(&actionCopyInstrBytes);

    initAction(&showInSubmenu, tr("Show in"), nullptr);
    addAction(&showInSubmenu);

    copySeparator = addSeparator();

    initAction(&actionAddComment, tr("Add Comment"), SLOT(on_actionAddComment_triggered()),
               getCommentSequence());
    addAction(&actionAddComment);

    initAction(&actionSetFunctionVarTypes, tr("Re-type Local Variables"),
               SLOT(on_actionSetFunctionVarTypes_triggered()), getRetypeSequence());
    addAction(&actionSetFunctionVarTypes);

    initAction(&actionEditFunction, tr("Edit function"), SLOT(on_actionEditFunction_triggered()),
               getEditFunctionSequence());
    addAction(&actionEditFunction);

    initAction(&actionDeleteComment, tr("Delete comment"),
               SLOT(on_actionDeleteComment_triggered()));
    addAction(&actionDeleteComment);

    initAction(&actionDeleteFlag, tr("Delete flag"), SLOT(on_actionDeleteFlag_triggered()));
    addAction(&actionDeleteFlag);

    initAction(&actionDeleteFunction, tr("Undefine function"),
               SLOT(on_actionDeleteFunction_triggered()), getUndefineFunctionSequence());
    addAction(&actionDeleteFunction);

    initAction(&actionAnalyzeFunction, tr("Define function here"),
               SLOT(on_actionAnalyzeFunction_triggered()), getDefineNewFunctionSequence());
    addAction(&actionAnalyzeFunction);

    addSeparator();

    addAddAtMenu();

    addSetBaseMenu();

    addSetBitsMenu();

    structureOffsetMenu = addMenu(tr("Structure offset"));
    connect(structureOffsetMenu, &QMenu::triggered, this,
            &DisassemblyContextMenu::on_actionStructureOffsetMenu_triggered);

    addSetAsMenu();

    addSeparator();

    initAction(&actionXRefs, tr("Show X-Refs"), SLOT(on_actionXRefs_triggered()),
               getXRefSequence());
    addAction(&actionXRefs);

    initAction(&actionXRefsForVariables, tr("X-Refs for local variables"),
               SLOT(on_actionXRefsForVariables_triggered()), QKeySequence(Qt::SHIFT | Qt::Key_X));
    addAction(&actionXRefsForVariables);

    initAction(&actionDisplayOptions, tr("Show Options"), SLOT(on_actionDisplayOptions_triggered()),
               getDisplayOptionsSequence());

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

    initAction(&actionRename, tr("Rename or add flag"), SLOT(on_actionRename_triggered()),
               getRenameSequence());
    setAsMenu->addAction(&actionRename);

    initAction(&actionGlobalVar, tr("Modify or add global variable"),
               SLOT(on_actionGlobalVar_triggered()), getGlobalVarSequence());
    setAsMenu->addAction(&actionGlobalVar);
}

void DisassemblyContextMenu::addSetBaseMenu()
{
    setBaseMenu = addMenu(tr("Set base of immediate value to.."));

    initAction(&actionSetBaseBinary, tr("Binary"));
    setBaseMenu->addAction(&actionSetBaseBinary);
    connect(&actionSetBaseBinary, &QAction::triggered, this, [this] { setBase("b"); });

    initAction(&actionSetBaseOctal, tr("Octal"));
    setBaseMenu->addAction(&actionSetBaseOctal);
    connect(&actionSetBaseOctal, &QAction::triggered, this, [this] { setBase("o"); });

    initAction(&actionSetBaseDecimal, tr("Decimal"));
    setBaseMenu->addAction(&actionSetBaseDecimal);
    connect(&actionSetBaseDecimal, &QAction::triggered, this, [this] { setBase("d"); });

    initAction(&actionSetBaseHexadecimal, tr("Hexadecimal"));
    setBaseMenu->addAction(&actionSetBaseHexadecimal);
    connect(&actionSetBaseHexadecimal, &QAction::triggered, this, [this] { setBase("h"); });

    initAction(&actionSetBasePort, tr("Network Port"));
    setBaseMenu->addAction(&actionSetBasePort);
    connect(&actionSetBasePort, &QAction::triggered, this, [this] { setBase("p"); });

    initAction(&actionSetBaseIPAddr, tr("IP Address"));
    setBaseMenu->addAction(&actionSetBaseIPAddr);
    connect(&actionSetBaseIPAddr, &QAction::triggered, this, [this] { setBase("i"); });

    initAction(&actionSetBaseSyscall, tr("Syscall"));
    setBaseMenu->addAction(&actionSetBaseSyscall);
    connect(&actionSetBaseSyscall, &QAction::triggered, this, [this] { setBase("S"); });

    initAction(&actionSetBaseString, tr("String"));
    setBaseMenu->addAction(&actionSetBaseString);
    connect(&actionSetBaseString, &QAction::triggered, this, [this] { setBase("s"); });
}

void DisassemblyContextMenu::addSetBitsMenu()
{
    setBitsMenu = addMenu(tr("Set current bits to..."));

    initAction(&actionSetBits16, "16");
    setBitsMenu->addAction(&actionSetBits16);
    connect(&actionSetBits16, &QAction::triggered, this, [this] { setBits(16); });

    initAction(&actionSetBits32, "32");
    setBitsMenu->addAction(&actionSetBits32);
    connect(&actionSetBits32, &QAction::triggered, this, [this] { setBits(32); });

    initAction(&actionSetBits64, "64");
    setBitsMenu->addAction(&actionSetBits64);
    connect(&actionSetBits64, &QAction::triggered, this, [this] { setBits(64); });
}

void DisassemblyContextMenu::addSetAsMenu()
{
    setAsMenu = addMenu(tr("Set as..."));

    initAction(&actionSetToCode, tr("Code"), SLOT(applySetToCode()),
               getSetToCodeSequence());
    setAsMenu->addAction(&actionSetToCode);

    setAsString = setAsMenu->addMenu(tr("String..."));

    initAction(&actionSetAsStringAuto, tr("Auto-detect"), SLOT(on_actionSetAsString_triggered()),
               getSetAsStringSequence());
    initAction(&actionSetAsStringRemove, tr("Remove"),
               SLOT(on_actionSetAsStringRemove_triggered()));
    initAction(&actionSetAsStringAdvanced, tr("Advanced"),
               SLOT(on_actionSetAsStringAdvanced_triggered()), getSetAsStringAdvanced());

    setAsString->addAction(&actionSetAsStringAuto);
    setAsString->addAction(&actionSetAsStringRemove);
    setAsString->addAction(&actionSetAsStringAdvanced);

    addSetToDataMenu();
}

void DisassemblyContextMenu::applySetToCode() {
    qDebug() << "Applying set to code" << "\n";
    if (selectedLines.size() > 1) {
        QVector<QPair<int, RVA>> offsets;
        for (const auto &selection : selectedLines) {
            int startPos = selection.cursor.selectionStart();
            RVA lineOffset = DisassemblyPreview::readDisassemblyOffset(selection.cursor);
            offsets.append(qMakePair(startPos, lineOffset));
        }

        // Sorting by start position in descending order
        std::sort(offsets.begin(), offsets.end(), 
            [](const QPair<int, RVA> &a, const QPair<int, RVA> &b) {
                return a.first > b.first;
            });

            for (const auto &offset : offsets) {
                qDebug() << "Offset:" << offset.second;
                on_actionSetToCode_triggered(offset.second);
            }
    } else if (selectedLines.size() <= 1) {
        on_actionSetToCode_triggered();
    }
    selectedLines.clear();
}

void DisassemblyContextMenu::addSetToDataMenu()
{
    setToDataMenu = setAsMenu->addMenu(tr("Data..."));

    initAction(&actionSetToDataByte, tr("Byte"));
    setToDataMenu->addAction(&actionSetToDataByte);
    connect(&actionSetToDataByte, &QAction::triggered, this, [this] { applySetToData(1); });

    initAction(&actionSetToDataWord, tr("Word"));
    setToDataMenu->addAction(&actionSetToDataWord);
    connect(&actionSetToDataWord, &QAction::triggered, this, [this] { applySetToData(2); });

    initAction(&actionSetToDataDword, tr("Dword"));
    setToDataMenu->addAction(&actionSetToDataDword);
    connect(&actionSetToDataDword, &QAction::triggered, this, [this] { applySetToData(4); });

    initAction(&actionSetToDataQword, tr("Qword"));
    setToDataMenu->addAction(&actionSetToDataQword);
    connect(&actionSetToDataQword, &QAction::triggered, this, [this] { applySetToData(8); });

    initAction(&actionSetToDataEx, "...", SLOT(on_actionSetToDataEx_triggered()), getSetToDataExSequence());
    setToDataMenu->addAction(&actionSetToDataEx);
}

void DisassemblyContextMenu::applySetToData(int dataSize) {
    qDebug() << "Applying set to data with size:" << dataSize << "\n";
    if (selectedLines.size() > 1) {
        QVector<QPair<int, RVA>> offsets;
        for (const auto &selection : selectedLines) {
            int startPos = selection.cursor.selectionStart();
            RVA lineOffset = DisassemblyPreview::readDisassemblyOffset(selection.cursor);
            offsets.append(qMakePair(startPos, lineOffset));
        }

        // Sorting by start position in descending order
        std::sort(offsets.begin(), offsets.end(), 
            [](const QPair<int, RVA> &a, const QPair<int, RVA> &b) {
                return a.first > b.first;
            });

        for (const auto &offset : offsets) {
            qDebug() << "Offset:" << offset.second;
            setToData(offset.second, dataSize);
        }
    } else if (selectedLines.size() <= 1) {
        setToData(dataSize);
    }
    selectedLines.clear();
}


void DisassemblyContextMenu::addEditMenu()
{
    editMenu = addMenu(tr("Edit"));

    initAction(&actionEditInstruction, tr("Instruction"),
               SLOT(on_actionEditInstruction_triggered()));
    editMenu->addAction(&actionEditInstruction);

    initAction(&actionNopInstruction, tr("Nop Instruction"),
               SLOT(on_actionNopInstruction_triggered()));
    editMenu->addAction(&actionNopInstruction);

    initAction(&actionEditBytes, tr("Bytes"), SLOT(on_actionEditBytes_triggered()));
    editMenu->addAction(&actionEditBytes);

    initAction(&actionJmpReverse, tr("Reverse Jump"), SLOT(on_actionJmpReverse_triggered()));
    editMenu->addAction(&actionJmpReverse);
}

void DisassemblyContextMenu::addBreakpointMenu()
{
    breakpointMenu = addMenu(tr("Breakpoint"));

    initAction(&actionAddBreakpoint, tr("Add/remove breakpoint"),
               SLOT(on_actionAddBreakpoint_triggered()), getAddBPSequence());
    breakpointMenu->addAction(&actionAddBreakpoint);
    initAction(&actionAdvancedBreakpoint, tr("Advanced breakpoint"),
               SLOT(on_actionAdvancedBreakpoint_triggered()), QKeySequence(Qt::CTRL | Qt::Key_F2));
    breakpointMenu->addAction(&actionAdvancedBreakpoint);
}

void DisassemblyContextMenu::addDebugMenu()
{
    debugMenu = addMenu(tr("Debug"));

    initAction(&actionContinueUntil, tr("Continue until line"),
               SLOT(on_actionContinueUntil_triggered()));
    debugMenu->addAction(&actionContinueUntil);

    initAction(&actionSetPC, "Set PC", SLOT(on_actionSetPC_triggered()));
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


void DisassemblyContextMenu::prepareMenu(const QList<QTextEdit::ExtraSelection>& selectedLines) {
    this->clear();
    this->selectedLines = selectedLines;

    qDebug() << "Number of selected lines:" << selectedLines.size();
    for (const auto& selection : selectedLines) {
        int cursorPosition = selection.cursor.position();
        int anchorPosition = selection.cursor.anchor();
        auto offset = DisassemblyPreview::readDisassemblyOffset(selection.cursor);

        qDebug() << "Cursor position:" << cursorPosition << "Anchor position:" << anchorPosition << "Offset:" << offset << "\n";
    }

    addAction(&actionCopy);

    if (selectedLines.size() > 1) {
        addSetAsMenu();
    }
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
    RzAnalysisFunction *fcn = Core()->functionAt(address);
    RzFlagItem *flag = rz_flag_get_i(Core()->core()->flags, address);

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
        doRenameInfo.name = RzAddressString(tuh->offset);
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
    ut64 selection = Core()->num(curHighlightedWord);

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
            for (auto v : vars) {
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

    auto name = RzAddressString(tuh->offset);
    actionGlobalVar.setText(tr("Add or change global variable at %1 (used here)").arg(name));

    actionRename.setVisible(true);
    actionGlobalVar.setVisible(true);
}

void DisassemblyContextMenu::aboutToShowSlot()
{
    // check if set immediate base menu makes sense
    auto ab = Core()->getRzAnalysisBytesSingle(offset);

    bool immBase = ab && ab->op && (ab->op->val || ab->op->ptr);
    setBaseMenu->menuAction()->setVisible(immBase);
    setBitsMenu->menuAction()->setVisible(true);

    // Create structure offset menu if it makes sense
    QString memBaseReg; // Base register
    st64 memDisp = 0; // Displacement

    if (ab && ab->op) {
        const char *opexstr = RZ_STRBUF_SAFEGET(&ab->op->opex);
        CutterJson operands = Core()->parseJson("opex", strdup(opexstr), nullptr);

        // Loop through both the operands of the instruction
        for (const CutterJson operand : operands) {
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
        RzList *typeoffs = rz_type_db_get_by_offset(core->analysis->typedb, memDisp);
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
    QString stringDefinition = Core()->getMetaString(offset);
    actionSetAsStringRemove.setVisible(!stringDefinition.isEmpty());

    QString comment = Core()->getCommentAt(offset);

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
    RzAnalysisFunction *in_fcn = Core()->functionIn(offset);
    if (in_fcn) {
        auto vars = Core()->getVariables(offset);
        actionSetFunctionVarTypes.setVisible(!vars.empty());
        actionEditFunction.setVisible(true);
        actionEditFunction.setText(tr("Edit function \"%1\"").arg(in_fcn->name));
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
    bool hasBreakpoint = Core()->breakpointIndexAt(offset) > -1;
    actionAddBreakpoint.setText(hasBreakpoint ? tr("Remove breakpoint") : tr("Add breakpoint"));
    actionAdvancedBreakpoint.setText(hasBreakpoint ? tr("Edit breakpoint")
                                                   : tr("Advanced breakpoint"));
    QString progCounterName = Core()->getRegisterName("PC").toUpper();
    actionSetPC.setText("Set " + progCounterName + " here");

    if (pluginMenu) {
        pluginActionMenuAction->setVisible(!pluginMenu->isEmpty());
        for (QAction *pluginAction : pluginMenu->actions()) {
            pluginAction->setData(QVariant::fromValue(offset));
        }
    }

    bool isLocalVar = isHighlightedWordLocalVar();
    actionXRefsForVariables.setVisible(isLocalVar);
    if (isLocalVar) {
        actionXRefsForVariables.setText(tr("X-Refs for %1").arg(curHighlightedWord));
    }
}

void DisassemblyContextMenu::aboutToHideSlot()
{
    actionXRefsForVariables.setVisible(true);
}

QKeySequence DisassemblyContextMenu::getCopySequence() const
{
    return QKeySequence::Copy;
}

QKeySequence DisassemblyContextMenu::getCommentSequence() const
{
    return { Qt::Key_Semicolon };
}

QKeySequence DisassemblyContextMenu::getCopyAddressSequence() const
{
    return { Qt::CTRL | Qt::SHIFT | Qt::Key_C };
}

QKeySequence DisassemblyContextMenu::getCopyInstrBytesSequence() const
{
    return { Qt::CTRL | Qt::ALT | Qt::Key_C };
}

QKeySequence DisassemblyContextMenu::getSetToCodeSequence() const
{
    return { Qt::Key_C };
}

QKeySequence DisassemblyContextMenu::getSetAsStringSequence() const
{
    return { Qt::Key_A };
}

QKeySequence DisassemblyContextMenu::getSetAsStringAdvanced() const
{
    return { Qt::SHIFT | Qt::Key_A };
}

QKeySequence DisassemblyContextMenu::getSetToDataSequence() const
{
    return { Qt::Key_D };
}

QKeySequence DisassemblyContextMenu::getSetToDataExSequence() const
{
    return { Qt::Key_Asterisk };
}

QKeySequence DisassemblyContextMenu::getRenameSequence() const
{
    return { Qt::Key_N };
}

QKeySequence DisassemblyContextMenu::getGlobalVarSequence() const
{
    return { Qt::Key_G };
}

QKeySequence DisassemblyContextMenu::getRetypeSequence() const
{
    return { Qt::Key_Y };
}

QKeySequence DisassemblyContextMenu::getXRefSequence() const
{
    return { Qt::Key_X };
}

QKeySequence DisassemblyContextMenu::getDisplayOptionsSequence() const
{
    return {}; // TODO insert correct sequence
}

QList<QKeySequence> DisassemblyContextMenu::getAddBPSequence() const
{
    return { Qt::Key_F2, Qt::CTRL | Qt::Key_B };
}

QKeySequence DisassemblyContextMenu::getDefineNewFunctionSequence() const
{
    return { Qt::Key_P };
}

QKeySequence DisassemblyContextMenu::getEditFunctionSequence() const
{
    return { Qt::SHIFT | Qt::Key_P };
}

QKeySequence DisassemblyContextMenu::getUndefineFunctionSequence() const
{
    return { Qt::Key_U };
}

void DisassemblyContextMenu::on_actionEditInstruction_triggered()
{
    if (!ioModesController.prepareForWriting()) {
        return;
    }
    EditInstructionDialog e(EDIT_TEXT, parentForDialog());
    e.setWindowTitle(tr("Edit Instruction at %1").arg(RzAddressString(offset)));

    QString oldInstructionOpcode = Core()->getInstructionOpcode(offset);
    QString oldInstructionBytes = Core()->getInstructionBytes(offset);

    e.setInstruction(oldInstructionOpcode);

    if (e.exec()) {
        bool fillWithNops = e.needsNops();
        QString userInstructionOpcode = e.getInstruction();
        if (userInstructionOpcode != oldInstructionOpcode) {
            Core()->editInstruction(offset, userInstructionOpcode, fillWithNops);
        }
    }
}

void DisassemblyContextMenu::on_actionNopInstruction_triggered()
{
    if (!ioModesController.prepareForWriting()) {
        return;
    }
    Core()->nopInstruction(offset);
}

void DisassemblyContextMenu::showReverseJmpQuery()
{
    actionJmpReverse.setVisible(false);
    auto ab = Core()->getRzAnalysisBytesSingle(offset);
    if (!(ab && ab->op)) {
        return;
    }
    if (ab->op->type == RZ_ANALYSIS_OP_TYPE_CJMP) {
        actionJmpReverse.setVisible(true);
    }
}

void DisassemblyContextMenu::on_actionJmpReverse_triggered()
{
    if (!ioModesController.prepareForWriting()) {
        return;
    }
    Core()->jmpReverse(offset);
}

void DisassemblyContextMenu::on_actionEditBytes_triggered()
{
    if (!ioModesController.prepareForWriting()) {
        return;
    }
    EditInstructionDialog e(EDIT_BYTES, parentForDialog());
    e.setWindowTitle(tr("Edit Bytes at %1").arg(RzAddressString(offset)));

    QString oldBytes = Core()->getInstructionBytes(offset);
    e.setInstruction(oldBytes);

    if (e.exec()) {
        QString bytes = e.getInstruction();
        if (bytes != oldBytes) {
            Core()->editBytes(offset, bytes);
        }
    }
}

void DisassemblyContextMenu::on_actionCopy_triggered()
{
    emit copy();
}

void DisassemblyContextMenu::on_actionCopyAddr_triggered()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(RzAddressString(offset));
}

void DisassemblyContextMenu::on_actionCopyInstrBytes_triggered()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(Core()->getInstructionBytes(offset));
}

void DisassemblyContextMenu::on_actionAddBreakpoint_triggered()
{
    Core()->toggleBreakpoint(offset);
}

void DisassemblyContextMenu::on_actionAdvancedBreakpoint_triggered()
{
    int index = Core()->breakpointIndexAt(offset);
    if (index >= 0) {
        BreakpointsDialog::editBreakpoint(Core()->getBreakpointAt(offset), parentForDialog());
    } else {
        BreakpointsDialog::createNewBreakpoint(offset, parentForDialog());
    }
}

void DisassemblyContextMenu::on_actionContinueUntil_triggered()
{
    Core()->continueUntilDebug(offset);
}

void DisassemblyContextMenu::on_actionSetPC_triggered()
{
    QString progCounterName = Core()->getRegisterName("PC");
    Core()->setRegister(progCounterName, RzAddressString(offset).toUpper());
}

void DisassemblyContextMenu::on_actionAddComment_triggered()
{
    CommentsDialog::addOrEditComment(offset, parentForDialog());
}

void DisassemblyContextMenu::on_actionAnalyzeFunction_triggered()
{
    RVA flagOffset;
    QString name = Core()->nearestFlag(offset, &flagOffset);
    if (name.isEmpty() || flagOffset != offset) {
        // Create a default name for the function
        QString pfx = Config()->getConfigString("analysis.fcnprefix");
        if (pfx.isEmpty()) {
            pfx = QString("fcn");
        }
        name = pfx + QString::asprintf(".%llx", offset);
    }

    // Create dialog
    QInputDialog inputDialog(parentForDialog());
    inputDialog.resize(500, 100);
    inputDialog.setWindowTitle(tr("New function at %1").arg(RzAddressString(offset)));
    inputDialog.setLabelText(tr("Function name:"));
    inputDialog.setTextValue(name);
    inputDialog.setWindowFlags(Qt::Window | Qt::WindowMinimizeButtonHint);

    if (inputDialog.exec() != QDialog::Accepted) {
        return;
    }

    QString functionName = inputDialog.textValue().trimmed();

    if (!functionName.isEmpty()) {
        Core()->createFunctionAt(offset, functionName);
    }
}

void DisassemblyContextMenu::on_actionRename_triggered()
{
    bool ok = false;
    if (doRenameAction == RENAME_FUNCTION) {
        QString newName = QInputDialog::getText(
                this->mainWindow, tr("Rename function %2").arg(doRenameInfo.name),
                tr("Function name:"), QLineEdit::Normal, doRenameInfo.name, &ok);
        if (ok && !newName.isEmpty()) {
            Core()->renameFunction(doRenameInfo.addr, newName);
        }
    } else if (doRenameAction == RENAME_FLAG || doRenameAction == RENAME_ADD_FLAG) {
        FlagDialog dialog(doRenameInfo.addr, parentForDialog());
        ok = dialog.exec();
    } else if (doRenameAction == RENAME_LOCAL) {
        RzAnalysisFunction *fcn = Core()->functionIn(offset);
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

void DisassemblyContextMenu::on_actionGlobalVar_triggered()
{
    bool ok = false;
    GlobalVariableDialog dialog(doRenameInfo.addr, parentForDialog());
    ok = dialog.exec();

    if (ok) {
        // Rebuild menu in case the user presses the rename shortcut directly before clicking
        setupRenaming();
    }
}

void DisassemblyContextMenu::on_actionSetFunctionVarTypes_triggered()
{
    RzAnalysisFunction *fcn = Core()->functionIn(offset);

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

void DisassemblyContextMenu::on_actionXRefs_triggered()
{
    XrefsDialog dialog(mainWindow);
    dialog.fillRefsForAddress(offset, RzAddressString(offset), false);
    dialog.exec();
}

void DisassemblyContextMenu::on_actionXRefsForVariables_triggered()
{
    if (isHighlightedWordLocalVar()) {
        XrefsDialog dialog(mainWindow);
        dialog.fillRefsForVariable(curHighlightedWord, offset);
        dialog.exec();
    }
}

void DisassemblyContextMenu::on_actionDisplayOptions_triggered()
{
    PreferencesDialog dialog(parentForDialog());
    dialog.showSection(PreferencesDialog::Section::Disassembly);
    dialog.exec();
}

void DisassemblyContextMenu::on_actionSetToCode_triggered()
{
    Core()->setToCode(offset);
}

void DisassemblyContextMenu::on_actionSetToCode_triggered(RVA offset)
{
    Core()->setToCode(offset);
}

void DisassemblyContextMenu::on_actionSetAsString_triggered()
{
    Core()->setAsString(offset);
}

void DisassemblyContextMenu::on_actionSetAsString_triggered(RVA offset)
{
    Core()->setAsString(offset);
}

void DisassemblyContextMenu::on_actionSetAsStringRemove_triggered()
{
    Core()->removeString(offset);
}

void DisassemblyContextMenu::on_actionSetAsStringRemove_triggered(RVA offset)
{
    Core()->removeString(offset);
}

void DisassemblyContextMenu::on_actionSetAsStringAdvanced_triggered()
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

void DisassemblyContextMenu::on_actionSetToData_triggered()
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

void DisassemblyContextMenu::on_actionSetToDataEx_triggered()
{
    SetToDataDialog dialog(offset, parentForDialog());
    if (!dialog.exec()) {
        return;
    }
    setToData(dialog.getItemSize(), dialog.getItemCount());
}

void DisassemblyContextMenu::on_actionStructureOffsetMenu_triggered(QAction *action)
{
    Core()->applyStructureOffset(action->data().toString(), offset);
}

void DisassemblyContextMenu::on_actionDeleteComment_triggered()
{
    Core()->delComment(offset);
}

void DisassemblyContextMenu::on_actionDeleteFlag_triggered()
{
    Core()->delFlag(offset);
}

void DisassemblyContextMenu::on_actionDeleteFunction_triggered()
{
    Core()->delFunction(offset);
}

void DisassemblyContextMenu::on_actionEditFunction_triggered()
{
    RzCore *core = Core()->core();
    EditFunctionDialog dialog(parentForDialog());
    RzAnalysisFunction *fcn = rz_analysis_get_fcn_in(core->analysis, offset, 0);

    if (fcn) {
        dialog.setWindowTitle(tr("Edit function %1").arg(fcn->name));
        dialog.setNameText(fcn->name);

        QString startAddrText = "0x" + QString::number(fcn->addr, 16);
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
            QString new_name = dialog.getNameText();
            rz_core_analysis_function_rename(core, fcn->addr, new_name.toStdString().c_str());
            QString new_start_addr = dialog.getStartAddrText();
            fcn->addr = Core()->math(new_start_addr);
            QString new_stack_size = dialog.getStackSizeText();
            fcn->stack = int(Core()->math(new_stack_size));

            QByteArray newCC = dialog.getCallConSelected().toUtf8();
            if (!newCC.isEmpty() && rz_analysis_cc_exist(core->analysis, newCC.constData())) {
                fcn->cc = rz_str_constpool_get(&core->analysis->constpool, newCC.constData());
            }

            emit Core()->functionsChanged();
        }
    }
}

void DisassemblyContextMenu::setBase(QString base)
{
    Core()->setImmediateBase(base, offset);
}

void DisassemblyContextMenu::setBits(int bits)
{
    Core()->setCurrentBits(bits, offset);
}

void DisassemblyContextMenu::setToData(int size, int repeat)
{
    Core()->setToData(offset, size, repeat);
}

void DisassemblyContextMenu::setToData(RVA offset, int size, int repeat)
{
    Core()->setToData(offset, size, repeat);
}

QAction *DisassemblyContextMenu::addAnonymousAction(QString name, const char *slot,
                                                    QKeySequence keySequence)
{
    auto action = new QAction(this);
    addAction(action);
    anonymousActions.append(action);
    initAction(action, name, slot, keySequence);
    return action;
}

void DisassemblyContextMenu::initAction(QAction *action, QString name, const char *slot)
{
    action->setParent(this);
    parentWidget()->addAction(action);
    action->setText(name);
    if (slot) {
        connect(action, SIGNAL(triggered(bool)), this, slot);
    }
}

void DisassemblyContextMenu::initAction(QAction *action, QString name, const char *slot,
                                        QKeySequence keySequence)
{
    initAction(action, name, slot);
    if (keySequence.isEmpty()) {
        return;
    }
    action->setShortcut(keySequence);
    action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
}

void DisassemblyContextMenu::initAction(QAction *action, QString name, const char *slot,
                                        QList<QKeySequence> keySequenceList)
{
    initAction(action, name, slot);
    if (keySequenceList.empty()) {
        return;
    }
    action->setShortcuts(keySequenceList);
    action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
}

bool DisassemblyContextMenu::isHighlightedWordLocalVar()
{
    QList<VariableDescription> variables = Core()->getVariables(offset);
    for (const VariableDescription &var : variables) {
        if (var.name == curHighlightedWord) {
            return true;
        }
    }
    return false;
}
