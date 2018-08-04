#ifndef DISASSEMBLYCONTEXTMENU_H
#define DISASSEMBLYCONTEXTMENU_H

#include "Cutter.h"
#include <QMenu>
#include <QKeySequence>

class DisassemblyContextMenu : public QMenu
{
    Q_OBJECT

public:
    DisassemblyContextMenu(QWidget *parent = nullptr);
    ~DisassemblyContextMenu();

signals:
    void copy();

public slots:
    void setOffset(RVA offset);
    void setCanCopy(bool enabled);

private slots:
    void aboutToShowSlot();

    void on_actionEditInstruction_triggered();
    void on_actionNopInstruction_triggered();
    void on_actionJmpReverse_triggered();
    void showReverseJmpQuery();
    void on_actionEditBytes_triggered();

    void on_actionCopy_triggered();
    void on_actionCopyAddr_triggered();
    void on_actionAddComment_triggered();
    void on_actionCreateFunction_triggered();
    void on_actionAddFlag_triggered();
    void on_actionRename_triggered();
    void on_actionRenameUsedHere_triggered();
    void on_actionXRefs_triggered();
    void on_actionDisplayOptions_triggered();

    void on_actionDeleteComment_triggered();
    void on_actionDeleteFlag_triggered();
    void on_actionDeleteFunction_triggered();

    void on_actionAddBreakpoint_triggered();
    void on_actionContinueUntil_triggered();
    void on_actionSetPC_triggered();

    void on_actionSetToCode_triggered();
    void on_actionSetToData_triggered();
    void on_actionSetToDataEx_triggered();

private:
    QKeySequence getCopySequence() const;
    QKeySequence getCommentSequence() const;
    QKeySequence getSetToCodeSequence() const;
    QKeySequence getSetToDataSequence() const;
    QKeySequence getSetToDataExSequence() const;
    QKeySequence getAddFlagSequence() const;
    QKeySequence getRenameSequence() const;
    QKeySequence getRenameUsedHereSequence() const;
    QKeySequence getXRefSequence() const;
    QKeySequence getDisplayOptionsSequence() const;
    QList<QKeySequence> getAddBPSequence() const;

    RVA offset;
    bool canCopy;

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
    QAction actionCreateFunction;
    QAction actionRename;
    QAction actionRenameUsedHere;
    QAction actionXRefs;
    QAction actionDisplayOptions;

    QAction actionDeleteComment;
    QAction actionDeleteFlag;
    QAction actionDeleteFunction;

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
    QAction actionAddBreakpoint;
    QAction actionSetPC;

    QAction actionSetToCode;

    QMenu *setToDataMenu;
    QAction actionSetToDataEx;
    QAction actionSetToDataByte;
    QAction actionSetToDataWord;
    QAction actionSetToDataDword;
    QAction actionSetToDataQword;

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
    void addSetToDataMenu();
    void addEditMenu();
    void addDebugMenu();
};
#endif // DISASSEMBLYCONTEXTMENU_H
