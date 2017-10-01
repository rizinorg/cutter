
#ifndef ASMOPTIONSDIALOG_H
#define ASMOPTIONSDIALOG_H

#include <QDialog>

#include "cutter.h"

namespace Ui
{
    class AsmOptionsDialog;
}

class AsmOptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AsmOptionsDialog(CutterCore *core, QWidget *parent = nullptr);
    ~AsmOptionsDialog();

private:
    CutterCore *core;

    Ui::AsmOptionsDialog *ui;

    void updateFromVars();

private slots:
    void on_esilCheckBox_toggled(bool checked);
    void on_pseudoCheckBox_toggled(bool checked);
    void on_offsetCheckBox_toggled(bool checked);
    void on_describeCheckBox_toggled(bool checked);
    void on_stackpointerCheckBox_toggled(bool checked);
    void on_bytesCheckBox_toggled(bool checked);
    void on_bytespaceCheckBox_toggled(bool checked);
    void on_lbytesCheckBox_toggled(bool checked);
    void on_syntaxComboBox_currentIndexChanged(int index);
    void on_uppercaseCheckBox_toggled(bool checked);

};


#endif //ASMOPTIONSDIALOG_H
