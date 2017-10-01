#ifndef FLAGDIALOG_H
#define FLAGDIALOG_H

#include <QDialog>
#include "cutter.h"

namespace Ui
{
    class FlagDialog;
}

class FlagDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FlagDialog(CutterCore *core, RVA offset, QWidget *parent = 0);
    ~FlagDialog();

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

private:
    Ui::FlagDialog *ui;

    CutterCore *core;
    RVA offset;
};

#endif // FLAGDIALOG_H
