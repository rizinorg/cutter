#ifndef DISASSEMBLYCONTEXTMENU_H
#define DISASSEMBLYCONTEXTMENU_H

#include "cutter.h"
#include <QMenu>
#include <QKeySequence>

class DisassemblyContextMenu : public QMenu
{
Q_OBJECT

public:
    DisassemblyContextMenu(QWidget *parent = nullptr);
    ~DisassemblyContextMenu() = default;

public slots:
    void setOffset(RVA offset);

private slots:
    void aboutToShowSlot();

    void on_actionAddComment_triggered();
    void on_actionAddFlag_triggered();
    void on_actionRename_triggered();
    void on_actionXRefs_triggered();
    void on_actionDisplayOptions_triggered();

    void on_actionSetBaseBinary_triggered();
    void on_actionSetBaseOctal_triggered();
    void on_actionSetBaseDecimal_triggered();
    void on_actionSetBaseHexadecimal_triggered();
    void on_actionSetBasePort_triggered();
    void on_actionSetBaseIPAddr_triggered();
    void on_actionSetBaseSyscall_triggered();
    void on_actionSetBaseString_triggered();

private:
    QKeySequence getCommentSequence() const;
    QKeySequence getAddFlagSequence() const;
    QKeySequence getRenameSequence() const;
    QKeySequence getXRefSequence() const;
    QKeySequence getDisplayOptionsSequence() const;

    RVA offset;

    QAction actionAddComment;
    QAction actionAddFlag;
    QAction actionRename;
    QAction actionXRefs;
    QAction actionDisplayOptions;

    QMenu *setBaseMenu;
    QAction *setBaseMenuAction;
    QAction actionSetBaseBinary;
    QAction actionSetBaseOctal;
    QAction actionSetBaseDecimal;
    QAction actionSetBaseHexadecimal;
    QAction actionSetBasePort;
    QAction actionSetBaseIPAddr;
    QAction actionSetBaseSyscall;
    QAction actionSetBaseString;
};
#endif // DISASSEMBLYCONTEXTMENU_H
