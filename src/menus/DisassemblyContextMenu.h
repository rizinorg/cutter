#ifndef DISASSEMBLYCONTEXTMENU_H
#define DISASSEMBLYCONTEXTMENU_H

#include "core/Cutter.h"
#include "common/IOModesController.h"
#include <QMenu>
#include <QKeySequence>

class MainWindow;

class CUTTER_EXPORT DisassemblyContextMenu : public QMenu
{
    Q_OBJECT

public:
    DisassemblyContextMenu(QWidget *parent, MainWindow *mainWindow);
    ~DisassemblyContextMenu();

signals:
    void copy();

public slots:
    void setOffset(RVA offset);
    void setCanCopy(bool enabled);

    /**
     * @brief Sets the value of curHighlightedWord
     * @param text The current highlighted word
     */
    void setCurHighlightedWord(const QString &text);

private slots:
    void aboutToShowSlot();
    void aboutToHideSlot();

    void on_actionEditFunction_triggered();
    void on_actionEditInstruction_triggered();
    void on_actionNopInstruction_triggered();
    void on_actionJmpReverse_triggered();
    void on_actionEditBytes_triggered();
    void showReverseJmpQuery();

    void on_actionCopy_triggered();
    void on_actionCopyAddr_triggered();
    void on_actionCopyInstrBytes_triggered();
    void on_actionAddComment_triggered();
    void on_actionAnalyzeFunction_triggered();
    void on_actionRename_triggered();
    void on_actionGlobalVar_triggered();
    void on_actionSetFunctionVarTypes_triggered();
    void on_actionXRefs_triggered();
    void on_actionXRefsForVariables_triggered();

    void on_actionDeleteComment_triggered();
    void on_actionDeleteFlag_triggered();
    void on_actionDeleteFunction_triggered();

    void on_actionAddBreakpoint_triggered();
    void on_actionAdvancedBreakpoint_triggered();
    void on_actionContinueUntil_triggered();
    void on_actionSetPC_triggered();

    void on_actionSetToCode_triggered();
    void on_actionSetAsString_triggered();
    void on_actionSetAsStringRemove_triggered();
    void on_actionSetAsStringAdvanced_triggered();
    void on_actionSetToData_triggered();
    void on_actionSetToDataEx_triggered();

    /**
     * @brief Executed on selecting an offset from the structureOffsetMenu
     * Uses the applyStructureOffset() function of CutterCore to apply the
     * structure offset
     * \param action The action which trigered the event
     */
    void on_actionStructureOffsetMenu_triggered(QAction *action);

private:
    RVA offset;
    bool canCopy;
    QString curHighlightedWord; // The current highlighted word
    MainWindow *mainWindow;
    IOModesController ioModesController;

    QList<QAction *> anonymousActions;

    QMenu *editMenu;
    QAction actionEditInstruction;
    QAction actionNopInstruction;
    QAction actionJmpReverse;
    QAction actionEditBytes;

    QAction actionCopy;
    QAction *copySeparator;
    QAction actionCopyAddr;
    QAction actionCopyInstrBytes;

    QAction actionAddComment;
    QAction actionAnalyzeFunction;
    QAction actionEditFunction;
    QAction actionRename;
    QAction actionGlobalVar;
    QAction actionSetFunctionVarTypes;
    QAction actionXRefs;
    QAction actionXRefsForVariables;

    QAction actionDeleteComment;
    QAction actionDeleteFlag;
    QAction actionDeleteFunction;

    QMenu *structureOffsetMenu;

    QMenu *setBaseMenu;
    QAction actionSetBaseBinary;
    QAction actionSetBaseOctal;
    QAction actionSetBaseDecimal;
    QAction actionSetBaseHexadecimal;
    QAction actionSetBasePort;
    QAction actionSetBaseIPAddr;
    QAction actionSetBaseSyscall;
    QAction actionSetBaseString;

    QMenu *setBitsMenu;
    QAction actionSetBits16;
    QAction actionSetBits32;
    QAction actionSetBits64;

    QMenu *debugMenu;
    QAction actionContinueUntil;
    QAction actionSetPC;

    QMenu *breakpointMenu;
    QAction actionAddBreakpoint;
    QAction actionAdvancedBreakpoint;

    QAction actionSetToCode;

    QAction actionSetAsStringAuto;
    QAction actionSetAsStringRemove;
    QAction actionSetAsStringAdvanced;

    QMenu *setToDataMenu;
    QMenu *setAsMenu;
    QMenu *setAsString;
    QAction actionSetToDataEx;
    QAction actionSetToDataByte;
    QAction actionSetToDataWord;
    QAction actionSetToDataDword;
    QAction actionSetToDataQword;

    QAction showInSubmenu;
    QList<QAction *> showTargetMenuActions;
    QMenu *pluginMenu = nullptr;
    QAction *pluginActionMenuAction = nullptr;

    /**
     * \return widget that should be used as parent for presenting dialogs
     */
    QWidget *parentForDialog();

    // For creating anonymous entries (that are always visible)
    QAction *addAnonymousAction(QString name, const char *slot, QKeySequence shortcut);

    void initAction(QAction *action, QString name, const char *slot = nullptr);
    void initAction(QAction *action, QString name, const char *slot, QKeySequence keySequence);
    void initShortcutAction(QAction *action, const QString &id, const char *slot);

    void setBase(QString base);
    void setToData(int size, int repeat = 1);
    void setBits(int bits);

    void addSetBaseMenu();
    void addSetBitsMenu();
    void addSetAsMenu();
    void addSetToDataMenu();
    void addEditMenu();
    void addAddAtMenu();
    void addBreakpointMenu();
    void addDebugMenu();

    enum DoRenameAction {
        RENAME_FUNCTION,
        RENAME_FLAG,
        RENAME_ADD_FLAG,
        RENAME_LOCAL,
        RENAME_DO_NOTHING,
    };
    struct DoRenameInfo
    {
        ut64 addr;
        QString name;
    };
    DoRenameAction doRenameAction = RENAME_DO_NOTHING;
    DoRenameInfo doRenameInfo = {};

    /*
     * @brief Setups up the "Rename" option in the context menu
     *
     * This function takes into account cursor location so it can choose between current address and
     * pointed value i.e. `0x000040f3  lea rdi, [0x000199b1]` -> does the user want to add a flag at
     * 0x40f3 or at 0x199b1? and for that we will rely on |curHighlightedWord| which is the
     * currently selected word.
     */
    void setupRenaming();

    /**
     * @brief Checks if the currently highlighted word in the disassembly widget
     * is a local variable or function paramter.
     * @return Return true if the highlighted word is the name of a local variable or function
     * parameter, return false otherwise.
     */
    bool isHighlightedWordLocalVar();
    struct ThingUsedHere
    {
        QString name;
        RVA offset;
        enum class Type { Var, Function, Flag, Address };
        Type type;
    };
    QVector<ThingUsedHere> getThingUsedHere(RVA offset);

    /*
     * @brief This function checks if the given address contains a function,
     * a flag or if it is just an address.
     */
    ThingUsedHere getThingAt(ut64 address);

    /*
     * @brief This function will set the text for the renaming menu given a ThingUsedHere
     * and provide information on how to handle the renaming of this specific thing.
     * Indeed, selected dialogs are different when it comes to adding a flag, renaming an existing
     * function, renaming a local variable...
     *
     * This function handles every possible object.
     */
    void buildRenameMenu(ThingUsedHere *tuh);
};
#endif // DISASSEMBLYCONTEXTMENU_H
