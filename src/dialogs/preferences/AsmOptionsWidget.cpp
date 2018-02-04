#include <QLabel>
#include <QFontDialog>

#include "AsmOptionsWidget.h"
#include "ui_AsmOptionsWidget.h"

#include "PreferencesDialog.h"

#include "utils/Helpers.h"
#include "utils/Configuration.h"

AsmOptionsWidget::AsmOptionsWidget(PreferencesDialog */*dialog*/, QWidget *parent)
  : QDialog(parent),
    ui(new Ui::AsmOptionsWidget)
{
    ui->setupUi(this);

    ui->syntaxComboBox->blockSignals(true);
    for(const auto &syntax : Core()->cmdList("e asm.syntax=?"))
        ui->syntaxComboBox->addItem(syntax, syntax);
    ui->syntaxComboBox->blockSignals(false);

    updateAsmOptionsFromVars();

    connect(Core(), SIGNAL(asmOptionsChanged()), this, SLOT(updateAsmOptionsFromVars()));

    ui->buttonBox->addButton(tr("Save as Defaults"), QDialogButtonBox::ButtonRole::ApplyRole);

    //connect(dialog, SIGNAL(saveAsDefault()), this, SLOT(saveAsDefault()));
    //connect(dialog, SIGNAL(resetToDefault()), this, SLOT(resetToDefault()));
}

AsmOptionsWidget::~AsmOptionsWidget() {}


void AsmOptionsWidget::updateAsmOptionsFromVars()
{
    qhelpers::setCheckedWithoutSignals(ui->esilCheckBox, Core()->getConfigb("asm.esil"));
    qhelpers::setCheckedWithoutSignals(ui->pseudoCheckBox, Core()->getConfigb("asm.pseudo"));
    qhelpers::setCheckedWithoutSignals(ui->offsetCheckBox, Core()->getConfigb("asm.offset"));
    qhelpers::setCheckedWithoutSignals(ui->describeCheckBox, Core()->getConfigb("asm.describe"));
    qhelpers::setCheckedWithoutSignals(ui->stackpointerCheckBox, Core()->getConfigb("asm.stackptr"));
    qhelpers::setCheckedWithoutSignals(ui->slowCheckBox, Core()->getConfigb("asm.slow"));
    qhelpers::setCheckedWithoutSignals(ui->linesCheckBox, Core()->getConfigb("asm.lines"));
    qhelpers::setCheckedWithoutSignals(ui->fcnlinesCheckBox, Core()->getConfigb("asm.fcnlines"));
    qhelpers::setCheckedWithoutSignals(ui->emuCheckBox, Core()->getConfigb("asm.emu"));
    qhelpers::setCheckedWithoutSignals(ui->cmtrightCheckBox, Core()->getConfigb("asm.cmtright"));
    qhelpers::setCheckedWithoutSignals(ui->varsumCheckBox, Core()->getConfigb("asm.varsum"));

    bool bytesEnabled = Core()->getConfigb("asm.bytes");
    qhelpers::setCheckedWithoutSignals(ui->bytesCheckBox, bytesEnabled);
    qhelpers::setCheckedWithoutSignals(ui->bytespaceCheckBox, Core()->getConfigb("asm.bytespace"));
    ui->bytespaceCheckBox->setEnabled(bytesEnabled);
    qhelpers::setCheckedWithoutSignals(ui->lbytesCheckBox, Core()->getConfigb("asm.lbytes"));
    ui->lbytesCheckBox->setEnabled(bytesEnabled);
    ui->nbytesSpinBox->blockSignals(true);
    ui->nbytesSpinBox->setValue(Core()->getConfigi("asm.nbytes"));
    ui->nbytesSpinBox->blockSignals(false);
    ui->nbytesLabel->setEnabled(bytesEnabled);
    ui->nbytesSpinBox->setEnabled(bytesEnabled);


    QString currentSyntax = Core()->getConfig("asm.syntax");
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
    if (Core()->getConfigb("asm.ucase"))
    {
        ui->caseComboBox->setCurrentIndex(1);
    }
    else if(Core()->getConfigb("asm.capitalize"))
    {
        ui->caseComboBox->setCurrentIndex(2);
    }
    else
    {
        ui->caseComboBox->setCurrentIndex(0);
    }
    ui->caseComboBox->blockSignals(false);

    ui->asmTabsSpinBox->blockSignals(true);
    ui->asmTabsSpinBox->setValue(Core()->getConfigi("asm.tabs"));
    ui->asmTabsSpinBox->blockSignals(false);

    qhelpers::setCheckedWithoutSignals(ui->bblineCheckBox, Core()->getConfigb("asm.bbline"));

    bool varsubEnabled = Core()->getConfigb("asm.varsub");
    qhelpers::setCheckedWithoutSignals(ui->varsubCheckBox, varsubEnabled);
    qhelpers::setCheckedWithoutSignals(ui->varsubOnlyCheckBox, Core()->getConfigb("asm.varsub_only"));
    ui->varsubOnlyCheckBox->setEnabled(varsubEnabled);
}


void AsmOptionsWidget::saveAsDefault()
{
    Core()->saveDefaultAsmOptions();
}

void AsmOptionsWidget::resetToDefault()
{
    Core()->resetDefaultAsmOptions();
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
    Core()->setConfig("asm.esil", checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_pseudoCheckBox_toggled(bool checked)
{
    Core()->setConfig("asm.pseudo", checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_offsetCheckBox_toggled(bool checked)
{
    Core()->setConfig("asm.offset", checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_describeCheckBox_toggled(bool checked)
{
    Core()->setConfig("asm.describe", checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_stackpointerCheckBox_toggled(bool checked)
{
    Core()->setConfig("asm.stackptr", checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_slowCheckBox_toggled(bool checked)
{
    Core()->setConfig("asm.slow", checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_linesCheckBox_toggled(bool checked)
{
    Core()->setConfig("asm.lines", checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_fcnlinesCheckBox_toggled(bool checked)
{
    Core()->setConfig("asm.fcnlines", checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_emuCheckBox_toggled(bool checked)
{
    Core()->setConfig("asm.emu", checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_cmtrightCheckBox_toggled(bool checked)
{
    Core()->setConfig("asm.cmtright", checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_varsumCheckBox_toggled(bool checked)
{
    Core()->setConfig("asm.varsum", checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_bytesCheckBox_toggled(bool checked)
{
    Core()->setConfig("asm.bytes", checked);
    ui->bytespaceCheckBox->setEnabled(checked);
    ui->lbytesCheckBox->setEnabled(checked);
    ui->nbytesLabel->setEnabled(checked);
    ui->nbytesSpinBox->setEnabled(checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_bytespaceCheckBox_toggled(bool checked)
{
    Core()->setConfig("asm.bytespace", checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_lbytesCheckBox_toggled(bool checked)
{
    Core()->setConfig("asm.lbytes", checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_nbytesSpinBox_valueChanged(int value)
{
    Core()->setConfig("asm.nbytes", value);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_syntaxComboBox_currentIndexChanged(int index)
{
    Core()->setConfig("asm.syntax", ui->syntaxComboBox->itemData(index).toString().toUtf8().constData());
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_caseComboBox_currentIndexChanged(int index)
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

    Core()->setConfig("asm.ucase", ucase);
    Core()->setConfig("asm.capitalize", capitalize);

    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_asmTabsSpinBox_valueChanged(int value)
{
    Core()->setConfig("asm.tabs", value);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_bblineCheckBox_toggled(bool checked)
{
    Core()->setConfig("asm.bbline", checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_varsubCheckBox_toggled(bool checked)
{
    Core()->setConfig("asm.varsub", checked);
    ui->varsubOnlyCheckBox->setEnabled(checked);
    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::on_varsubOnlyCheckBox_toggled(bool checked)
{
    Core()->setConfig("asm.varsub_only", checked);
    triggerAsmOptionsChanged();
}


void AsmOptionsWidget::on_buttonBox_clicked(QAbstractButton *button)
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
