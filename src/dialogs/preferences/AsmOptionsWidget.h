
#ifndef ASMOPTIONSWIDGET_H
#define ASMOPTIONSWIDGET_H

#include <QDialog>
#include <QPushButton>
#include <memory>

#include "Cutter.h"

class PreferencesDialog;

namespace Ui {
class AsmOptionsWidget;
}

class AsmOptionsWidget : public QDialog
{
    Q_OBJECT

public:
    explicit AsmOptionsWidget(PreferencesDialog *dialog, QWidget *parent = nullptr);
    ~AsmOptionsWidget();

private:
    std::unique_ptr<Ui::AsmOptionsWidget> ui;

    void triggerAsmOptionsChanged();

private slots:
    void resetToDefault();

    void updateAsmOptionsFromVars();

    void on_esilCheckBox_toggled(bool checked);
    void on_pseudoCheckBox_toggled(bool checked);
    void on_offsetCheckBox_toggled(bool checked);
    void on_describeCheckBox_toggled(bool checked);
    void on_stackpointerCheckBox_toggled(bool checked);
    void on_slowCheckBox_toggled(bool checked);
    void on_linesCheckBox_toggled(bool checked);
    void on_fcnlinesCheckBox_toggled(bool checked);
    void on_flgoffCheckBox_toggled(bool checked);
    void on_emuCheckBox_toggled(bool checked);
    void on_cmtrightCheckBox_toggled(bool checked);
    void on_cmtcolSpinBox_valueChanged(int value);
    void on_varsumCheckBox_toggled(bool checked);
    void on_bytesCheckBox_toggled(bool checked);
    void on_sizeCheckBox_toggled(bool checked);
    void on_bytespaceCheckBox_toggled(bool checked);
    void on_lbytesCheckBox_toggled(bool checked);
    void on_syntaxComboBox_currentIndexChanged(int index);
    void on_caseComboBox_currentIndexChanged(int index);
    void on_asmTabsSpinBox_valueChanged(int value);
    void on_asmTabsOffSpinBox_valueChanged(int value);
    void on_nbytesSpinBox_valueChanged(int value);
    void on_bblineCheckBox_toggled(bool checked);
    void on_varsubCheckBox_toggled(bool checked);
    void on_varsubOnlyCheckBox_toggled(bool checked);

    void on_buttonBox_clicked(QAbstractButton *button);
};


#endif //ASMOPTIONSWIDGET_H
