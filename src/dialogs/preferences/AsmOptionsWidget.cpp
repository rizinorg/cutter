#include <QLabel>
#include <QFontDialog>

#include "AsmOptionsWidget.h"
#include "ui_AsmOptionsWidget.h"

#include "PreferencesDialog.h"

#include "utils/Helpers.h"
#include "utils/Configuration.h"

AsmOptionsWidget::AsmOptionsWidget(PreferencesDialog */*dialog*/, QWidget *parent)
    : AbstractOptionWidget(parent),
      ui(new Ui::AsmOptionsWidget)
{
    ui->setupUi(this);

    ui->syntaxComboBox->blockSignals(true);
    for (const auto &syntax : Core()->cmdList("e asm.syntax=?"))
        ui->syntaxComboBox->addItem(syntax, syntax);
    ui->syntaxComboBox->blockSignals(false);

    updateAsmOptionsFromVars();

    connect(Core(), SIGNAL(asmOptionsChanged()), this, SLOT(updateAsmOptionsFromVars()));
}

AsmOptionsWidget::~AsmOptionsWidget() {}

void AsmOptionsWidget::apply()
{
    for (auto &it : currSettings.keys())
        Config()->setConfig(it, currSettings.value(it));
    updateAsmOptionsFromVars();
    isChanged = false;
}

void AsmOptionsWidget::discard()
{
    for (auto &it : currSettings.keys())
        currSettings.setValue(it, Config()->getConfigVar(it));
    updateAsmOptionsFromVars();
    isChanged = false;
}

void AsmOptionsWidget::updateAsmOptionsFromVars()
{
    qhelpers::setCheckedWithoutSignals(ui->esilCheckBox, Config()->getConfigBool("asm.esil"));
    currSettings.setValue("asm.esil", Config()->getConfigBool("asm.esil"));
    qhelpers::setCheckedWithoutSignals(ui->pseudoCheckBox, Config()->getConfigBool("asm.pseudo"));
    currSettings.setValue("asm.pseudo", Config()->getConfigBool("asm.pseudo"));
    qhelpers::setCheckedWithoutSignals(ui->offsetCheckBox, Config()->getConfigBool("asm.offset"));
    currSettings.setValue("asm.offset", Config()->getConfigBool("asm.offset"));
    qhelpers::setCheckedWithoutSignals(ui->describeCheckBox, Config()->getConfigBool("asm.describe"));
    currSettings.setValue("asm.describe", Config()->getConfigBool("asm.describe"));
    qhelpers::setCheckedWithoutSignals(ui->stackpointerCheckBox, Config()->getConfigBool("asm.stackptr"));
    currSettings.setValue("asm.stackptr", Config()->getConfigBool("asm.stackptr"));
    qhelpers::setCheckedWithoutSignals(ui->slowCheckBox, Config()->getConfigBool("asm.slow"));
    currSettings.setValue("asm.slow", Config()->getConfigBool("asm.slow"));
    qhelpers::setCheckedWithoutSignals(ui->linesCheckBox, Config()->getConfigBool("asm.lines"));
    currSettings.setValue("asm.lines", Config()->getConfigBool("asm.lines"));
    qhelpers::setCheckedWithoutSignals(ui->fcnlinesCheckBox, Config()->getConfigBool("asm.lines.fcn"));
    currSettings.setValue("asm.lines.fcn", Config()->getConfigBool("asm.lines.fcn"));
    qhelpers::setCheckedWithoutSignals(ui->flgoffCheckBox, Config()->getConfigBool("asm.flags.offset"));
    currSettings.setValue("asm.flags.offset", Config()->getConfigBool("asm.flags.offset"));
    qhelpers::setCheckedWithoutSignals(ui->emuCheckBox, Config()->getConfigBool("asm.emu"));
    currSettings.setValue("asm.emu", Config()->getConfigBool("asm.emu"));
    qhelpers::setCheckedWithoutSignals(ui->varsumCheckBox, Config()->getConfigBool("asm.var.summary"));
    currSettings.setValue("asm.var.summary", Config()->getConfigBool("asm.var.summary"));
    qhelpers::setCheckedWithoutSignals(ui->sizeCheckBox, Config()->getConfigBool("asm.size"));
    currSettings.setValue("asm.size", Config()->getConfigBool("asm.size"));

    bool cmtRightEnabled = Config()->getConfigBool("asm.cmt.right");
    qhelpers::setCheckedWithoutSignals(ui->cmtrightCheckBox, cmtRightEnabled);
    currSettings.setValue("asm.cmt.right", cmtRightEnabled);
    ui->cmtcolSpinBox->blockSignals(true);
    ui->cmtcolSpinBox->setValue(Config()->getConfigInt("asm.cmt.col"));
    currSettings.setValue("asm.cmt.col", Config()->getConfigInt("asm.cmt.col"));
    ui->cmtcolSpinBox->blockSignals(false);
    ui->cmtcolSpinBox->setEnabled(cmtRightEnabled);

    bool bytesEnabled = Config()->getConfigBool("asm.bytes");
    currSettings.setValue("asm.bytes", bytesEnabled);
    qhelpers::setCheckedWithoutSignals(ui->bytesCheckBox, bytesEnabled);
    qhelpers::setCheckedWithoutSignals(ui->bytespaceCheckBox, Config()->getConfigBool("asm.bytespace"));
    currSettings.setValue("asm.bytespace", Config()->getConfigBool("asm.bytespace"));
    ui->bytespaceCheckBox->setEnabled(bytesEnabled);
    qhelpers::setCheckedWithoutSignals(ui->lbytesCheckBox, Config()->getConfigBool("asm.lbytes"));
    currSettings.setValue("asm.lbytes", Config()->getConfigBool("asm.lbytes"));
    ui->lbytesCheckBox->setEnabled(bytesEnabled);
    ui->nbytesSpinBox->blockSignals(true);
    ui->nbytesSpinBox->setValue(Config()->getConfigInt("asm.nbytes"));
    currSettings.setValue("asm.nbytes", Config()->getConfigInt("asm.nbytes"));

    ui->nbytesSpinBox->blockSignals(false);
    ui->nbytesLabel->setEnabled(bytesEnabled);
    ui->nbytesSpinBox->setEnabled(bytesEnabled);


    QString currentSyntax = Config()->getConfigString("asm.syntax");
    currSettings.setValue("asm.syntax", Config()->getConfigString("asm.syntax"));
    for (int i = 0; i < ui->syntaxComboBox->count(); i++) {
        if (ui->syntaxComboBox->itemData(i) == currentSyntax) {
            ui->syntaxComboBox->blockSignals(true);
            ui->syntaxComboBox->setCurrentIndex(i);
            ui->syntaxComboBox->blockSignals(false);
            break;
        }
    }

    ui->caseComboBox->blockSignals(true);
    if (Config()->getConfigBool("asm.ucase")) {
        ui->caseComboBox->setCurrentIndex(1);
    } else if (Config()->getConfigBool("asm.capitalize")) {
        ui->caseComboBox->setCurrentIndex(2);
    } else {
        ui->caseComboBox->setCurrentIndex(0);
    }
    ui->caseComboBox->blockSignals(false);
    currSettings.setValue("asm.ucase", Config()->getConfigBool("asm.ucase"));
    currSettings.setValue("asm.capitalize", Config()->getConfigBool("asm.capitalize"));

    ui->asmTabsSpinBox->blockSignals(true);
    ui->asmTabsSpinBox->setValue(Config()->getConfigInt("asm.tabs"));
    currSettings.setValue("asm.tabs", Config()->getConfigInt("asm.tabs"));
    ui->asmTabsSpinBox->blockSignals(false);


    ui->asmTabsOffSpinBox->blockSignals(true);
    ui->asmTabsOffSpinBox->setValue(Config()->getConfigInt("asm.tabs.off"));
    currSettings.setValue("asm.tabs.off", Config()->getConfigInt("asm.tabs.off"));
    ui->asmTabsOffSpinBox->blockSignals(false);

    qhelpers::setCheckedWithoutSignals(ui->bblineCheckBox, Config()->getConfigBool("asm.bbline"));
    currSettings.setValue("asm.bbline", Config()->getConfigBool("asm.bbline"));

    bool varsubEnabled = Config()->getConfigBool("asm.var.sub");
    currSettings.setValue("asm.var.sub", Config()->getConfigBool("asm.var.sub"));
    qhelpers::setCheckedWithoutSignals(ui->varsubCheckBox, varsubEnabled);
    qhelpers::setCheckedWithoutSignals(ui->varsubOnlyCheckBox, Config()->getConfigBool("asm.var.subonly"));
    currSettings.setValue("asm.var.subonly", Config()->getConfigBool("asm.var.subonly"));
    ui->varsubOnlyCheckBox->setEnabled(varsubEnabled);
}

void AsmOptionsWidget::resetToDefault()
{
    Config()->resetToDefaultAsmOptions();
    QSettings tmp;
    for (auto &it : currSettings.keys())
        tmp.setValue(it, currSettings.value(it));
    updateAsmOptionsFromVars();
    for (auto &it : tmp.allKeys())
        Config()->setConfig(it, tmp.value(it));
    isChanged = true;
}

void AsmOptionsWidget::triggerAsmOptionsChanged()
{
    disconnect(Core(), SIGNAL(asmOptionsChanged()), this, SLOT(updateAsmOptionsFromVars()));
    Core()->triggerAsmOptionsChanged();
    connect(Core(), SIGNAL(asmOptionsChanged()), this, SLOT(updateAsmOptionsFromVars()));
}


void AsmOptionsWidget::on_esilCheckBox_toggled(bool checked)
{
    currSettings.setValue("asm.esil", checked);
    isChanged = true;
}

void AsmOptionsWidget::on_pseudoCheckBox_toggled(bool checked)
{
    currSettings.setValue("asm.pseudo", checked);
    isChanged = true;
}

void AsmOptionsWidget::on_offsetCheckBox_toggled(bool checked)
{
    currSettings.setValue("asm.offset", checked);
    isChanged = true;
}

void AsmOptionsWidget::on_describeCheckBox_toggled(bool checked)
{
    currSettings.setValue("asm.describe", checked);
    isChanged = true;
}

void AsmOptionsWidget::on_stackpointerCheckBox_toggled(bool checked)
{
    currSettings.setValue("asm.stackptr", checked);
    isChanged = true;
}

void AsmOptionsWidget::on_slowCheckBox_toggled(bool checked)
{
    currSettings.setValue("asm.slow", checked);
    isChanged = true;
}

void AsmOptionsWidget::on_linesCheckBox_toggled(bool checked)
{
    currSettings.setValue("asm.lines", checked);
    isChanged = true;
}

void AsmOptionsWidget::on_fcnlinesCheckBox_toggled(bool checked)
{
    currSettings.setValue("asm.lines.fcn", checked);
    isChanged = true;
}

void AsmOptionsWidget::on_flgoffCheckBox_toggled(bool checked)
{
    currSettings.setValue("asm.flags.off", checked);
    isChanged = true;
}

void AsmOptionsWidget::on_emuCheckBox_toggled(bool checked)
{
    currSettings.setValue("asm.emu", checked);
    isChanged = true;
}

void AsmOptionsWidget::on_cmtrightCheckBox_toggled(bool checked)
{
    currSettings.setValue("asm.cmt.right", checked);
    ui->cmtcolSpinBox->setEnabled(checked);
    isChanged = true;
}

void AsmOptionsWidget::on_cmtcolSpinBox_valueChanged(int value)
{
    currSettings.setValue("asm.cmt.col", value);
    isChanged = true;
}

void AsmOptionsWidget::on_varsumCheckBox_toggled(bool checked)
{
    currSettings.setValue("asm.var.summary", checked);
    isChanged = true;
}

void AsmOptionsWidget::on_bytesCheckBox_toggled(bool checked)
{
    currSettings.setValue("asm.bytes", checked);
    ui->bytespaceCheckBox->setEnabled(checked);
    ui->lbytesCheckBox->setEnabled(checked);
    ui->nbytesLabel->setEnabled(checked);
    ui->nbytesSpinBox->setEnabled(checked);
    isChanged = true;
}

void AsmOptionsWidget::on_sizeCheckBox_toggled(bool checked)
{
    currSettings.setValue("asm.size", checked);
    isChanged = true;
}

void AsmOptionsWidget::on_bytespaceCheckBox_toggled(bool checked)
{
    currSettings.setValue("asm.bytespace", checked);
    isChanged = true;
}

void AsmOptionsWidget::on_lbytesCheckBox_toggled(bool checked)
{
    currSettings.setValue("asm.lbytes", checked);
    isChanged = true;
}

void AsmOptionsWidget::on_nbytesSpinBox_valueChanged(int value)
{
    currSettings.setValue("asm.nbytes", value);
    isChanged = true;
}

void AsmOptionsWidget::on_syntaxComboBox_currentIndexChanged(int index)
{
    currSettings.setValue("asm.syntax",
                      ui->syntaxComboBox->itemData(index).toString().toUtf8().constData());
    isChanged = true;
}

void AsmOptionsWidget::on_caseComboBox_currentIndexChanged(int index)
{
    bool ucase;
    bool capitalize;

    switch (index) {
    // lowercase
    case 0:
    default:
        ucase = false;
        capitalize = false;
        break;

    // uppercase
    case 1:
        ucase = true;
        capitalize = false;
        break;

    case 2:
        ucase = false;
        capitalize = true;
        break;
    }

    currSettings.setValue("asm.ucase", ucase);
    currSettings.setValue("asm.capitalize", capitalize);

    isChanged = true;
}

void AsmOptionsWidget::on_asmTabsSpinBox_valueChanged(int value)
{
    currSettings.setValue("asm.tabs", value);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_asmTabsOffSpinBox_valueChanged(int value)
{
    currSettings.setValue("asm.tabs.off", value);
    isChanged = true;
}

void AsmOptionsWidget::on_bblineCheckBox_toggled(bool checked)
{
    currSettings.setValue("asm.bbline", checked);
    isChanged = true;
}

void AsmOptionsWidget::on_varsubCheckBox_toggled(bool checked)
{
    currSettings.setValue("asm.var.sub", checked);
    ui->varsubOnlyCheckBox->setEnabled(checked);
    isChanged = true;
}

void AsmOptionsWidget::on_varsubOnlyCheckBox_toggled(bool checked)
{
    currSettings.setValue("asm.var.subonly", checked);
    isChanged = true;
}


void AsmOptionsWidget::on_buttonBox_clicked(QAbstractButton *button)
{
    switch (ui->buttonBox->buttonRole(button)) {
    case QDialogButtonBox::ButtonRole::ResetRole:
        resetToDefault();
        break;
    default:
        break;
    }
}
