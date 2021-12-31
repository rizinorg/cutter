#ifndef ARENAINFODIALOG_H
#define ARENAINFODIALOG_H

#include <QDialog>
#include <CutterDescriptions.h>

namespace Ui {
class ArenaInfoDialog;
}

class ArenaInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ArenaInfoDialog(Arena &arena, QWidget *parent = nullptr);
    ~ArenaInfoDialog();
    void updateContents();

private:
    Ui::ArenaInfoDialog *ui;
    Arena arena;
};

#endif // ARENAINFODIALOG_H
