#ifndef DISASSEMBLYCONTEXTMENU_H
#define DISASSEMBLYCONTEXTMENU_H

#include "core/Cutter.h"
#include <QMenu>
#include <QKeySequence>

class DisassemblyContextMenu : public QMenu
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

    void on_actionEditFunction_triggered();
    void on_actionEditInstruction_triggered();
    void on_actionNopInstruction_triggered();
    void on_actionJmpReverse_triggered();
    void on_actionEditBytes_triggered();
    void showReverseJmpQuery();
    bool writeFailed();

    void on_actionCopy_triggered();
    void on_actionCopyAddr_triggered();
    void on_actionAddComment_triggered();
    void on_actionAnalyzeFunction_triggered();
    void on_actionAddFlag_triggered();
    void on_actionRename_triggered();
    void on_actionRenameUsedHere_triggered();
    void on_actionSetFunctionVarTypes_triggered();
    void on_actionXRefs_triggered();
    void on_actionDisplayOptions_triggered();

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

    /**
     * @brief Executed on selecting the "Link Type to Address" option
     * Opens the LinkTypeDialog box from where the user can link the address
     * to a type
     */
    void on_actionLinkType_triggered();

private:
    QKeySequence getCopySequence() const;
    QKeySequence getCommentSequence() const;
    QKeySequence getCopyAddressSequence() const;
    QKeySequence getSetToCodeSequence() const;
    QKeySequence getSetAsStringSequence() const;
    QKeySequence getSetToDataSequence() const;
    QKeySequence getSetToDataExSequence() const;
    QKeySequence getAddFlagSequence() const;
    QKeySequence getRenameSequence() const;
    QKeySequence getRenameUsedHereSequence() const;
    QKeySequence getRetypeSequence() const;
    QKeySequence getXRefSequence() const;
    QKeySequence getDisplayOptionsSequence() const;
    QKeySequence getDefineNewFunctionSequence() const;
    QKeySequence getUndefineFunctionSequence() const;
    QKeySequence getEditFunctionSequence() const;
    QList<QKeySequence> getAddBPSequence() const;

    /**
     * @return the shortcut key for "Link Type to Address" option
     */
    QKeySequence getLinkTypeSequence() const;


    RVA offset;
    bool canCopy;
    QString curHighlightedWord; // The current highlighted word
    MainWindow *mainWindow;

    QList<QAction *> anonymousActions;

    QMenu *editMenu;
    QAction actionEditInstruction;
    QAction actionNopInstruction;
    QAction actionJmpReverse;
    QAction actionEditBytes;

    QAction actionCopy;
    QAction *copySeparator;
    QAction actionCopyAddr;


    QAction actionAddComment;
    QAction actionAddFlag;
    QAction actionAnalyzeFunction;
    QAction actionEditFunction;
    QAction actionRename;
    QAction actionRenameUsedHere;
    QAction actionSetFunctionVarTypes;
    QAction actionXRefs;
    QAction actionDisplayOptions;

    QAction actionDeleteComment;
    QAction actionDeleteFlag;
    QAction actionDeleteFunction;

    QMenu *structureOffsetMenu;

    QAction actionLinkType;

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
    QList<QAction*> showTargetMenuActions;
    QMenu *pluginMenu = nullptr;
    QAction *pluginActionMenuAction = nullptr;

    // For creating anonymous entries (that are always visible)
    QAction *addAnonymousAction(QString name, const char *slot, QKeySequence shortcut);

    void initAction(QAction *action, QString name, const char *slot = nullptr);
    void initAction(QAction *action, QString name, const char *slot, QKeySequence keySequence);
    void initAction(QAction *action, QString name, const char *slot, QList<QKeySequence> keySequence);

    void setBase(QString base);
    void setToData(int size, int repeat = 1);
    void setBits(int bits);

    void addSetBaseMenu();
    void addSetBitsMenu();
    void addSetAsMenu();
    void addSetToDataMenu();
    void addEditMenu();
    void addBreakpointMenu();
    void addDebugMenu();

    struct ThingUsedHere {
        QString name;
        RVA offset;
        enum class Type {
            Var,
            Function,
            Flag,
            Address
        };
        Type type;
    };
    QVector<ThingUsedHere> getThingUsedHere(RVA offset);

    void updateTargetMenuActions(const QVector<ThingUsedHere> &targets);
};
#endif // DISASSEMBLYCONTEXTMENU_H
