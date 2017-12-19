
#ifndef ASMOPTIONSWIDGET_H
#define ASMOPTIONSWIDGET_H

#include <QDialog>
#include <QPushButton>
#include <memory>

#include "cutter.h"

class PreferencesDialog;

namespace Ui
{
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
    void saveAsDefault();
    void resetToDefault();

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
};


#endif //ASMOPTIONSWIDGET_H
