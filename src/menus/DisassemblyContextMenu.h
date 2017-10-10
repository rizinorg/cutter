#ifndef DISASSEMBLYCONTEXTMENU_H
#define DISASSEMBLYCONTEXTMENU_H

#include "cutter.h"
#include <QMenu>

class DisassemblyContextMenu : public QMenu {
Q_OBJECT
public:
    DisassemblyContextMenu(RVA offset, QWidget *parent = nullptr);
    ~DisassemblyContextMenu();

private:
    RVA offset;
    QAction actionAddComment;
    QAction actionAddFlag;
    QAction actionRename;
    QAction actionXRefs;
    QAction actionDisplayOptions;

private slots:
    void on_actionAddComment_triggered();
    void on_actionAddFlag_triggered();
    void on_actionRename_triggered();
    void on_actionXRefs_triggered();
    void on_actionDisplayOptions_triggered();
};
#endif // DISASSEMBLYCONTEXTMENU_H
