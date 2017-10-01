
#include "asmoptionsdialog.h"
#include "ui_asmoptionsdialog.h"

#include "helpers.h"

AsmOptionsDialog::AsmOptionsDialog(CutterCore *core, QWidget *parent) : QDialog(parent), ui(new Ui::AsmOptionsDialog)
{
    this->core = core;

    ui->setupUi(this);

    ui->syntaxComboBox->blockSignals(true);
    for(const auto &syntax : core->cmdList("e asm.syntax=?"))
        ui->syntaxComboBox->addItem(syntax, syntax);
    ui->syntaxComboBox->blockSignals(false);

    updateFromVars();
}

AsmOptionsDialog::~AsmOptionsDialog()
{
    delete ui;
}

void AsmOptionsDialog::updateFromVars()
{
    qhelpers::setCheckedWithoutSignals(ui->esilCheckBox, core->getConfigb("asm.esil"));
    qhelpers::setCheckedWithoutSignals(ui->pseudoCheckBox, core->getConfigb("asm.pseudo"));
    qhelpers::setCheckedWithoutSignals(ui->offsetCheckBox, core->getConfigb("asm.offset"));
    qhelpers::setCheckedWithoutSignals(ui->describeCheckBox, core->getConfigb("asm.describe"));
    qhelpers::setCheckedWithoutSignals(ui->stackpointerCheckBox, core->getConfigb("asm.stackptr"));

    bool bytesEnabled = core->getConfigb("asm.bytes");
    qhelpers::setCheckedWithoutSignals(ui->bytesCheckBox, bytesEnabled);
    qhelpers::setCheckedWithoutSignals(ui->bytespaceCheckBox, core->getConfigb("asm.bytespace"));
    ui->bytespaceCheckBox->setEnabled(bytesEnabled);
    qhelpers::setCheckedWithoutSignals(ui->lbytesCheckBox, core->getConfigb("asm.lbytes"));
    ui->lbytesCheckBox->setEnabled(bytesEnabled);

    QString currentSyntax = core->getConfig("asm.syntax");
    for (int i = 0; i < ui->syntaxComboBox->count(); i++)
    {
        if (ui->syntaxComboBox->itemData(i) == currentSyntax)
        {
            ui->syntaxComboBox->blockSignals(true);
            ui->syntaxComboBox->setCurrentIndex(i);
            ui->syntaxComboBox->blockSignals(false);
            break;
        }
    }

    qhelpers::setCheckedWithoutSignals(ui->uppercaseCheckBox, core->getConfigb("asm.ucase"));
}

void AsmOptionsDialog::on_esilCheckBox_toggled(bool checked)
{
    core->setConfig("asm.esil", checked);
    core->triggerAsmOptionsChanged();
}

void AsmOptionsDialog::on_pseudoCheckBox_toggled(bool checked)
{
    core->setConfig("asm.pseudo", checked);
    core->triggerAsmOptionsChanged();
}

void AsmOptionsDialog::on_offsetCheckBox_toggled(bool checked)
{
    core->setConfig("asm.offset", checked);
    core->triggerAsmOptionsChanged();
}

void AsmOptionsDialog::on_describeCheckBox_toggled(bool checked)
{
    core->setConfig("asm.describe", checked);
    core->triggerAsmOptionsChanged();
}

void AsmOptionsDialog::on_stackpointerCheckBox_toggled(bool checked)
{
    core->setConfig("asm.stackptr", checked);
    core->triggerAsmOptionsChanged();
}

void AsmOptionsDialog::on_bytesCheckBox_toggled(bool checked)
{
    core->setConfig("asm.bytes", checked);
    ui->bytespaceCheckBox->setEnabled(checked);
    ui->lbytesCheckBox->setEnabled(checked);
    core->triggerAsmOptionsChanged();
}

void AsmOptionsDialog::on_bytespaceCheckBox_toggled(bool checked)
{
    core->setConfig("asm.bytespace", checked);
    core->triggerAsmOptionsChanged();
}

void AsmOptionsDialog::on_lbytesCheckBox_toggled(bool checked)
{
    core->setConfig("asm.lbytes", checked);
    core->triggerAsmOptionsChanged();
}

void AsmOptionsDialog::on_syntaxComboBox_currentIndexChanged(int index)
{
    core->setConfig("asm.syntax", ui->syntaxComboBox->itemData(index).toString().toUtf8().constData());
    core->triggerAsmOptionsChanged();
}

void AsmOptionsDialog::on_uppercaseCheckBox_toggled(bool checked)
{
    core->setConfig("asm.ucase", checked);
    core->triggerAsmOptionsChanged();
}
