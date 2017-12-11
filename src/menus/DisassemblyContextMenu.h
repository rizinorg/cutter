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
    ~DisassemblyContextMenu();

signals:
    void copy();

public slots:
    void setOffset(RVA offset);
    void setCanCopy(bool enabled);

private slots:
    void aboutToShowSlot();

    void on_actionCopy_triggered();

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

    void on_actionSetBaseBinary_triggered();
    void on_actionSetBaseOctal_triggered();
    void on_actionSetBaseDecimal_triggered();
    void on_actionSetBaseHexadecimal_triggered();
    void on_actionSetBasePort_triggered();
    void on_actionSetBaseIPAddr_triggered();
    void on_actionSetBaseSyscall_triggered();
    void on_actionSetBaseString_triggered();

private:
    QKeySequence getCopySequence() const;
    QKeySequence getCommentSequence() const;
    QKeySequence getAddFlagSequence() const;
    QKeySequence getRenameSequence() const;
    QKeySequence getRenameUsedHereSequence() const;
    QKeySequence getXRefSequence() const;
    QKeySequence getDisplayOptionsSequence() const;

    RVA offset;
    bool canCopy;

    QList<QAction*> anonymousActions;

    QAction actionCopy;
    QAction *copySeparator;



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
    QAction *setBaseMenuAction;
    QAction actionSetBaseBinary;
    QAction actionSetBaseOctal;
    QAction actionSetBaseDecimal;
    QAction actionSetBaseHexadecimal;
    QAction actionSetBasePort;
    QAction actionSetBaseIPAddr;
    QAction actionSetBaseSyscall;
    QAction actionSetBaseString;

    // For creating anonymous entries (that are always visible)
    void createAction(QString name, QKeySequence keySequence, const char *slot);
    void createAction(QAction *action, QString name, QKeySequence keySequence, const char *slot);
};
#endif // DISASSEMBLYCONTEXTMENU_H
