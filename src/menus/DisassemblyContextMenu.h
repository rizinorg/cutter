#ifndef DISASSEMBLYCONTEXTMENU_H
#define DISASSEMBLYCONTEXTMENU_H

#include "common/IOModesController.h"

#include <QKeySequence>
#include <QMenu>

class MainWindow;

/**
 * @brief Context menu for @ref DisassemblyWidget and @ref DisassemblerGraphView
 */
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

    void editFunctionTriggered();
    void editInstructionTriggered();
    void nopInstructionTriggered();
    void jmpReverseTriggered();
    void editBytesTriggered();
    void showReverseJmpQuery();

    void copyTriggered();
    void copyAddrTriggered() const;
    void copyInstrBytesTriggered() const;
    void addCommentTriggered();
    void analyzeFunctionTriggered();
    void renameTriggered();
    void globalVarTriggered();
    void setFunctionVarTypesTriggered();
    void xRefsTriggered();
    void xRefsForVariablesTriggered();

    void deleteCommentTriggered() const;
    void deleteFlagTriggered() const;
    void deleteFunctionTriggered() const;

    void addBreakpointTriggered() const;
    void advancedBreakpointTriggered();
    void continueUntilTriggered() const;
    void setPCTriggered() const;

    void setToCodeTriggered() const;
    void setAsStringTriggered() const;
    void setAsStringRemoveTriggered() const;
    void setAsStringAdvancedTriggered();
    void setToDataTriggered();
    void setToDataExTriggered();

    /**
     * @brief Executed on selecting an offset from the structureOffsetMenu
     * Uses the applyStructureOffset() function of CutterCore to apply the
     * structure offset
     * @param action The action which trigered the event
     */
    void structureOffsetMenuTriggered(QAction *action) const;

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
     * @return widget that should be used as parent for presenting dialogs
     */
    QWidget *parentForDialog();

    // For creating anonymous entries (that are always visible)
    template<typename SlotFunc>
    QAction *addAnonymousAction(QString name, SlotFunc slot, QKeySequence keySequence);

    template<typename SlotFunc>
    void initAction(QAction *action, const QString &name, SlotFunc slot);

    template<typename SlotFunc>
    void initAction(QAction *action, QString name, SlotFunc slot, const QKeySequence &keySequence);

    template<typename SlotFunc>
    void initShortcutAction(QAction *action, const QString &id, SlotFunc slot);

    void setBase(const QString &base) const;
    void setToData(int size, int repeat = 1) const;
    void setBits(int bits) const;

    void addSetBaseMenu();
    void addSetBitsMenu();
    void addSetAsMenu();
    void addSetToDataMenu();
    void addEditMenu();
    void addAddAtMenu();
    void addBreakpointMenu();
    void addDebugMenu();

    enum DoRenameAction : ut8 {
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
        enum class Type : ut8 { Var, Function, Flag, Address };
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
