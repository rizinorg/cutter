#include "DisassemblyContextMenu.h"
#include "dialogs/AsmOptionsDialog.h"
#include "dialogs/CommentsDialog.h"
#include "dialogs/RenameDialog.h"
#include "dialogs/XrefsDialog.h"
#include <QtCore>

DisassemblyContextMenu::DisassemblyContextMenu(QWidget *parent) :
    QMenu(parent)
{
    actionAddComment.setText("Add comment");
    this->addAction(&actionAddComment);
    actionAddFlag.setText("Add flag");
    this->addAction(&actionAddFlag);
    actionRename.setText("Rename");
    this->addAction(&actionRename);
    actionXRefs.setText("Show xrefs");
    this->addAction(&actionXRefs);
    this->addSeparator();
    actionDisplayOptions.setText("Show options");
    this->addAction(&actionDisplayOptions);

    connect(&actionAddComment, SIGNAL(triggered()), this, SIGNAL(addComment_triggered()));
    connect(&actionXRefs, SIGNAL(triggered()), this, SIGNAL(xRefs_triggered()));
    connect(&actionAddFlag, SIGNAL(triggered()), this, SIGNAL(addFlag_triggered()));
    connect(&actionRename, SIGNAL(triggered()), this, SIGNAL(rename_triggered()));
    connect(&actionDisplayOptions, SIGNAL(triggered()), this, SIGNAL(displayOptions_triggered()));
}

DisassemblyContextMenu::~DisassemblyContextMenu()
{
}
