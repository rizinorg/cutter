#include <QLabel>
#include <QFontDialog>

#include "AsmOptionsDialog.h"
#include "ui_AsmOptionsDialog.h"

#include "utils/Helpers.h"
#include "utils/Configuration.h"

AsmOptionsDialog::AsmOptionsDialog(QWidget *parent)
  : QDialog(parent),
    ui(new Ui::AsmOptionsDialog),
    core(CutterCore::getInstance())
{
    ui->setupUi(this);

    ui->buttonBox->addButton(tr("Save as Defaults"), QDialogButtonBox::ButtonRole::ApplyRole);

    ui->syntaxComboBox->blockSignals(true);
    for(const auto &syntax : core->cmdList("e asm.syntax=?"))
        ui->syntaxComboBox->addItem(syntax, syntax);
    ui->syntaxComboBox->blockSignals(false);

    QFont currentFont = Config()->getFont();
    ui->fontSelectionLabel->setText(currentFont.toString());

    updateFromVars();
}

AsmOptionsDialog::~AsmOptionsDialog() {}

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

    ui->caseComboBox->blockSignals(true);
    if (core->getConfigb("asm.ucase"))
    {
        ui->caseComboBox->setCurrentIndex(1);
    }
    else if(core->getConfigb("asm.capitalize"))
    {
        ui->caseComboBox->setCurrentIndex(2);
    }
    else
    {
        ui->caseComboBox->setCurrentIndex(0);
    }
    ui->caseComboBox->blockSignals(false);

    qhelpers::setCheckedWithoutSignals(ui->bblineCheckBox, core->getConfigb("asm.bbline"));

    bool varsubEnabled = core->getConfigb("asm.varsub");
    qhelpers::setCheckedWithoutSignals(ui->varsubCheckBox, varsubEnabled);
    qhelpers::setCheckedWithoutSignals(ui->varsubOnlyCheckBox, core->getConfigb("asm.varsub_only"));
    ui->varsubOnlyCheckBox->setEnabled(varsubEnabled);
}


void AsmOptionsDialog::saveAsDefault()
{
    core->saveDefaultAsmOptions();
}

void AsmOptionsDialog::resetToDefault()
{
    core->resetDefaultAsmOptions();
    updateFromVars();
    core->triggerAsmOptionsChanged();
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

void AsmOptionsDialog::on_caseComboBox_currentIndexChanged(int index)
{
    bool ucase;
    bool capitalize;

    switch (index)
    {
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

    core->setConfig("asm.ucase", ucase);
    core->setConfig("asm.capitalize", capitalize);

    core->triggerAsmOptionsChanged();
}

void AsmOptionsDialog::on_bblineCheckBox_toggled(bool checked)
{
    core->setConfig("asm.bbline", checked);
    core->triggerAsmOptionsChanged();
}

void AsmOptionsDialog::on_varsubCheckBox_toggled(bool checked)
{
    core->setConfig("asm.varsub", checked);
    ui->varsubOnlyCheckBox->setEnabled(checked);
    core->triggerAsmOptionsChanged();
}

void AsmOptionsDialog::on_varsubOnlyCheckBox_toggled(bool checked)
{
    core->setConfig("asm.varsub_only", checked);
    core->triggerAsmOptionsChanged();
}

void AsmOptionsDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    switch (ui->buttonBox->buttonRole(button))
    {
        case QDialogButtonBox::ButtonRole::ApplyRole:
            saveAsDefault();
            break;
        case QDialogButtonBox::ButtonRole::ResetRole:
            resetToDefault();
            break;
        default:
            break;
    }
}

void AsmOptionsDialog::on_fontSelectionButton_clicked()
{
    QFont currentFont = Config()->getFont();
    bool ok;
    QFont newFont = QFontDialog::getFont(&ok, currentFont, this);
    if (ok) {
        Config()->setFont(newFont);
        ui->fontSelectionLabel->setText(newFont.toString());
    }
}
