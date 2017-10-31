#ifndef DISASSEMBLYCONTEXTMENU_H
#define DISASSEMBLYCONTEXTMENU_H

#include "cutter.h"
#include <QMenu>

class DisassemblyContextMenu : public QMenu {
Q_OBJECT
public:
    DisassemblyContextMenu(QWidget *parent = nullptr);
    ~DisassemblyContextMenu();


signals:
    void addComment_triggered();
    void xRefs_triggered();
    void addFlag_triggered();
    void rename_triggered();
    void displayOptions_triggered();

private:
    RVA offset;
    QAction actionAddComment;
    QAction actionAddFlag;
    QAction actionRename;
    QAction actionXRefs;
    QAction actionDisplayOptions;
};
#endif // DISASSEMBLYCONTEXTMENU_H
