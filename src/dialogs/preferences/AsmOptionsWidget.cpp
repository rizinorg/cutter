#include <QLabel>
#include <QFontDialog>

#include "AsmOptionsWidget.h"
#include "ui_AsmOptionsWidget.h"

#include "PreferencesDialog.h"

#include "common/Helpers.h"
#include "common/Configuration.h"

AsmOptionsWidget::AsmOptionsWidget(PreferencesDialog *dialog)
    : QDialog(dialog),
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


void AsmOptionsWidget::updateAsmOptionsFromVars()
{
    qhelpers::setCheckedWithoutSignals(ui->esilCheckBox, Config()->getConfigBool("asm.esil"));
    qhelpers::setCheckedWithoutSignals(ui->pseudoCheckBox, Config()->getConfigBool("asm.pseudo"));
    qhelpers::setCheckedWithoutSignals(ui->offsetCheckBox, Config()->getConfigBool("asm.offset"));
    qhelpers::setCheckedWithoutSignals(ui->xrefCheckBox, Config()->getConfigBool("asm.xrefs"));
    qhelpers::setCheckedWithoutSignals(ui->indentCheckBox, Config()->getConfigBool("asm.indent"));
    qhelpers::setCheckedWithoutSignals(ui->describeCheckBox, Config()->getConfigBool("asm.describe"));
    qhelpers::setCheckedWithoutSignals(ui->slowCheckBox, Config()->getConfigBool("asm.slow"));
    qhelpers::setCheckedWithoutSignals(ui->linesCheckBox, Config()->getConfigBool("asm.lines"));
    qhelpers::setCheckedWithoutSignals(ui->fcnlinesCheckBox, Config()->getConfigBool("asm.lines.fcn"));
    qhelpers::setCheckedWithoutSignals(ui->flgoffCheckBox, Config()->getConfigBool("asm.flags.offset"));
    qhelpers::setCheckedWithoutSignals(ui->emuCheckBox, Config()->getConfigBool("asm.emu"));
    qhelpers::setCheckedWithoutSignals(ui->emuStrCheckBox, Config()->getConfigBool("emu.str"));
    qhelpers::setCheckedWithoutSignals(ui->varsumCheckBox, Config()->getConfigBool("asm.var.summary"));
    qhelpers::setCheckedWithoutSignals(ui->sizeCheckBox, Config()->getConfigBool("asm.size"));

    bool cmtRightEnabled = Config()->getConfigBool("asm.cmt.right");
    qhelpers::setCheckedWithoutSignals(ui->cmtrightCheckBox, cmtRightEnabled);
    ui->cmtcolSpinBox->blockSignals(true);
    ui->cmtcolSpinBox->setValue(Config()->getConfigInt("asm.cmt.col"));
    ui->cmtcolSpinBox->blockSignals(false);
    ui->cmtcolSpinBox->setEnabled(cmtRightEnabled);

    bool bytesEnabled = Config()->getConfigBool("asm.bytes");
    qhelpers::setCheckedWithoutSignals(ui->bytesCheckBox, bytesEnabled);
    qhelpers::setCheckedWithoutSignals(ui->bytespaceCheckBox, Config()->getConfigBool("asm.bytespace"));
    ui->bytespaceCheckBox->setEnabled(bytesEnabled);
    qhelpers::setCheckedWithoutSignals(ui->lbytesCheckBox, Config()->getConfigBool("asm.lbytes"));
    ui->lbytesCheckBox->setEnabled(bytesEnabled);
    ui->nbytesSpinBox->blockSignals(true);
    ui->nbytesSpinBox->setValue(Config()->getConfigInt("asm.nbytes"));
    ui->nbytesSpinBox->blockSignals(false);
    ui->nbytesLabel->setEnabled(bytesEnabled);
    ui->nbytesSpinBox->setEnabled(bytesEnabled);


    QString currentSyntax = Config()->getConfigString("asm.syntax");
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

    ui->asmTabsSpinBox->blockSignals(true);
    ui->asmTabsSpinBox->setValue(Config()->getConfigInt("asm.tabs"));
    ui->asmTabsSpinBox->blockSignals(false);

    ui->asmTabsOffSpinBox->blockSignals(true);
    ui->asmTabsOffSpinBox->setValue(Config()->getConfigInt("asm.tabs.off"));
    ui->asmTabsOffSpinBox->blockSignals(false);

    qhelpers::setCheckedWithoutSignals(ui->bblineCheckBox, Config()->getConfigBool("asm.bb.line"));

    bool varsubEnabled = Config()->getConfigBool("asm.var.sub");
    qhelpers::setCheckedWithoutSignals(ui->varsubCheckBox, varsubEnabled);
    qhelpers::setCheckedWithoutSignals(ui->varsubOnlyCheckBox,
                                       Config()->getConfigBool("asm.var.subonly"));
    ui->varsubOnlyCheckBox->setEnabled(varsubEnabled);
}

void AsmOptionsWidget::resetToDefault()
{
    Config()->resetToDefaultAsmOptions();
    updateAsmOptionsFromVars();
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::triggerAsmOptionsChanged()
{
    disconnect(Core(), SIGNAL(asmOptionsChanged()), this, SLOT(updateAsmOptionsFromVars()));
    Core()->triggerAsmOptionsChanged();
    connect(Core(), SIGNAL(asmOptionsChanged()), this, SLOT(updateAsmOptionsFromVars()));
}


void AsmOptionsWidget::on_esilCheckBox_toggled(bool checked)
{
    Config()->setConfig("asm.esil", checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_pseudoCheckBox_toggled(bool checked)
{
    Config()->setConfig("asm.pseudo", checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_offsetCheckBox_toggled(bool checked)
{
    Config()->setConfig("asm.offset", checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_xrefCheckBox_toggled(bool checked)
{
    Config()->setConfig("asm.xrefs", checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_indentCheckBox_toggled(bool checked)
{
    Config()->setConfig("asm.indent", checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_describeCheckBox_toggled(bool checked)
{
    Config()->setConfig("asm.describe", checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_slowCheckBox_toggled(bool checked)
{
    Config()->setConfig("asm.slow", checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_linesCheckBox_toggled(bool checked)
{
    Config()->setConfig("asm.lines", checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_fcnlinesCheckBox_toggled(bool checked)
{
    Config()->setConfig("asm.lines.fcn", checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_flgoffCheckBox_toggled(bool checked)
{
    Config()->setConfig("asm.flags.off", checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_emuCheckBox_toggled(bool checked)
{
    Config()->setConfig("asm.emu", checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_emuStrCheckBox_toggled(bool checked)
{
    Config()->setConfig("emu.str", checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_cmtrightCheckBox_toggled(bool checked)
{
    Config()->setConfig("asm.cmt.right", checked);
    ui->cmtcolSpinBox->setEnabled(checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_cmtcolSpinBox_valueChanged(int value)
{
    Config()->setConfig("asm.cmt.col", value);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_varsumCheckBox_toggled(bool checked)
{
    Config()->setConfig("asm.var.summary", checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_bytesCheckBox_toggled(bool checked)
{
    Config()->setConfig("asm.bytes", checked);
    ui->bytespaceCheckBox->setEnabled(checked);
    ui->lbytesCheckBox->setEnabled(checked);
    ui->nbytesLabel->setEnabled(checked);
    ui->nbytesSpinBox->setEnabled(checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_sizeCheckBox_toggled(bool checked)
{
    Config()->setConfig("asm.size", checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_bytespaceCheckBox_toggled(bool checked)
{
    Config()->setConfig("asm.bytespace", checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_lbytesCheckBox_toggled(bool checked)
{
    Config()->setConfig("asm.lbytes", checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_nbytesSpinBox_valueChanged(int value)
{
    Config()->setConfig("asm.nbytes", value);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_syntaxComboBox_currentIndexChanged(int index)
{
    Config()->setConfig("asm.syntax",
                        ui->syntaxComboBox->itemData(index).toString().toUtf8().constData());
    triggerAsmOptionsChanged();
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

    Config()->setConfig("asm.ucase", ucase);
    Config()->setConfig("asm.capitalize", capitalize);

    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_asmTabsSpinBox_valueChanged(int value)
{
    Config()->setConfig("asm.tabs", value);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_asmTabsOffSpinBox_valueChanged(int value)
{
    Config()->setConfig("asm.tabs.off", value);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_bblineCheckBox_toggled(bool checked)
{
    Config()->setConfig("asm.bb.line", checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_varsubCheckBox_toggled(bool checked)
{
    Config()->setConfig("asm.var.sub", checked);
    ui->varsubOnlyCheckBox->setEnabled(checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_varsubOnlyCheckBox_toggled(bool checked)
{
    Config()->setConfig("asm.var.subonly", checked);
    triggerAsmOptionsChanged();
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
