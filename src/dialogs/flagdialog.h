#ifndef FLAGDIALOG_H
#define FLAGDIALOG_H

#include <QDialog>
#include <iaitorcore.h>

namespace Ui
{
    class FlagDialog;
}

class FlagDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FlagDialog(IaitoRCore *core, RVA offset, QWidget *parent = 0);
    ~FlagDialog();

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

private:
    Ui::FlagDialog *ui;

    IaitoRCore *core;
    RVA offset;
};

#endif // FLAGDIALOG_H
