
#ifndef ASMOPTIONSDIALOG_H
#define ASMOPTIONSDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <memory>

#include "cutter.h"

namespace Ui
{
    class AsmOptionsDialog;
}

class AsmOptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AsmOptionsDialog(QWidget *parent = nullptr);
    ~AsmOptionsDialog();

private:
    CutterCore *core;
    std::unique_ptr<Ui::AsmOptionsDialog> ui;

    void saveAsDefault();
    void resetToDefault();

    void triggerAsmOptionsChanged();

private slots:
    void updateFontFromConfig();
    void updateAsmOptionsFromVars();

    void on_esilCheckBox_toggled(bool checked);
    void on_pseudoCheckBox_toggled(bool checked);
    void on_offsetCheckBox_toggled(bool checked);
    void on_describeCheckBox_toggled(bool checked);
    void on_stackpointerCheckBox_toggled(bool checked);
    void on_bytesCheckBox_toggled(bool checked);
    void on_bytespaceCheckBox_toggled(bool checked);
    void on_lbytesCheckBox_toggled(bool checked);
    void on_syntaxComboBox_currentIndexChanged(int index);
    void on_caseComboBox_currentIndexChanged(int index);
    void on_bblineCheckBox_toggled(bool checked);
    void on_varsubCheckBox_toggled(bool checked);
    void on_varsubOnlyCheckBox_toggled(bool checked);
    void on_buttonBox_clicked(QAbstractButton *button);
    void on_fontSelectionButton_clicked();
};


#endif //ASMOPTIONSDIALOG_H
