#ifndef ARENAINFODIALOG_H
#define ARENAINFODIALOG_H

#include <QDialog>

namespace Ui {
class ArenaInfoDialog;
}

class ArenaInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ArenaInfoDialog(QWidget *parent = nullptr);
    ~ArenaInfoDialog();

private:
    Ui::ArenaInfoDialog *ui;
};

#endif // ARENAINFODIALOG_H
