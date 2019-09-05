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

    checkboxes = {
        { ui->describeCheckBox,     "asm.describe" },
        { ui->refptrCheckBox,       "asm.refptr" },
        { ui->xrefCheckBox,         "asm.xrefs" },
        { ui->bblineCheckBox,       "asm.bb.line" },
        { ui->varsubCheckBox,       "asm.var.sub" },
        { ui->varsubOnlyCheckBox,   "asm.var.subonly" },
        { ui->lbytesCheckBox,       "asm.lbytes" },
        { ui->bytespaceCheckBox,    "asm.bytespace" },
        { ui->bytesCheckBox,        "asm.bytes" },
        { ui->xrefCheckBox,         "asm.xrefs" },
        { ui->indentCheckBox,       "asm.indent" },
        { ui->offsetCheckBox,       "asm.offset" },
        { ui->slowCheckBox,         "asm.slow" },
        { ui->linesCheckBox,        "asm.lines" },
        { ui->fcnlinesCheckBox,     "asm.lines.fcn" },
        { ui->flgoffCheckBox,       "asm.flags.offset" },
        { ui->emuCheckBox,          "asm.emu" },
        { ui->emuStrCheckBox,       "emu.str" },
        { ui->varsumCheckBox,       "asm.var.summary" },
        { ui->sizeCheckBox,         "asm.size" },
    };


    QList<ConfigCheckbox>::iterator confCheckbox;

    // Connect each checkbox from "checkboxes" to the generic signal "checkboxEnabler"
    for (confCheckbox = checkboxes.begin(); confCheckbox != checkboxes.end(); ++confCheckbox) {
        QString val = confCheckbox->config;
        QCheckBox &cb = *confCheckbox->checkBox;
        connect(confCheckbox->checkBox, &QCheckBox::stateChanged, [this, val, &cb]() { checkboxEnabler(&cb, val) ;});
    }

    connect(ui->commentsComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                &AsmOptionsWidget::commentsComboBoxChanged);
    connect(ui->asmComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                &AsmOptionsWidget::asmComboBoxChanged);
    connect(Core(), SIGNAL(asmOptionsChanged()), this, SLOT(updateAsmOptionsFromVars()));
    updateAsmOptionsFromVars();
}

AsmOptionsWidget::~AsmOptionsWidget() {}


void AsmOptionsWidget::updateAsmOptionsFromVars()
{
    bool cmtRightEnabled = Config()->getConfigBool("asm.cmt.right");
    ui->cmtcolSpinBox->blockSignals(true);
    ui->cmtcolSpinBox->setValue(Config()->getConfigInt("asm.cmt.col"));
    ui->cmtcolSpinBox->blockSignals(false);
    ui->cmtcolSpinBox->setEnabled(cmtRightEnabled);

    bool bytesEnabled = Config()->getConfigBool("asm.bytes");
    ui->bytespaceCheckBox->setEnabled(bytesEnabled);
    ui->lbytesCheckBox->setEnabled(bytesEnabled);
    ui->nbytesSpinBox->blockSignals(true);
    ui->nbytesSpinBox->setValue(Config()->getConfigInt("asm.nbytes"));
    ui->nbytesSpinBox->blockSignals(false);
    ui->nbytesLabel->setEnabled(bytesEnabled);
    ui->nbytesSpinBox->setEnabled(bytesEnabled);
    bool varsubEnabled = Config()->getConfigBool("asm.var.sub");
    ui->varsubOnlyCheckBox->setEnabled(varsubEnabled);

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


    QList<ConfigCheckbox>::iterator confCheckbox;

    // Set the value for each checkbox in "checkboxes" as it exists in the configuration
    for (confCheckbox = checkboxes.begin(); confCheckbox != checkboxes.end(); ++confCheckbox) {
        qhelpers::setCheckedWithoutSignals(confCheckbox->checkBox,  Config()->getConfigBool(confCheckbox->config));
    }

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

void AsmOptionsWidget::on_cmtcolSpinBox_valueChanged(int value)
{
    Config()->setConfig("asm.cmt.col", value);
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


void AsmOptionsWidget::on_varsubCheckBox_toggled(bool checked)
{
    Config()->setConfig("asm.var.sub", checked);
    ui->varsubOnlyCheckBox->setEnabled(checked);
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

void AsmOptionsWidget::commentsComboBoxChanged(int index)
{
    // Check if comments should be set to right
    Config()->setConfig("asm.cmt.right", index != 1);
    // Check if comments are disabled
    ui->cmtcolSpinBox->setEnabled(index != 1);

    // Show\Hide comments in disassembly based on whether "Off" is selected
    Config()->setConfig("asm.comments", index != 2);
    // Enable comments-related checkboxes only if Comments are enabled
    ui->xrefCheckBox->setEnabled(index != 2);
    ui->refptrCheckBox->setEnabled(index != 2);
    ui->describeCheckBox->setEnabled(index != 2);

    triggerAsmOptionsChanged();
}

void AsmOptionsWidget::asmComboBoxChanged(int index)
{
    // Check if ESIL enabled
    Config()->setConfig("asm.esil", index == 1);

    // Check if Pseudocode enabled
    Config()->setConfig("asm.pseudo", index == 2);
    triggerAsmOptionsChanged();
}

/**
 * @brief A generic signal to handle the simple cases where a checkbox is toggled
 * while it only responsible for a single independent boolean configuration eval.
 * @param checkBox - The checkbox which is responsible for the siganl
 * @param config - the configuration string to be toggled
 */
void AsmOptionsWidget::checkboxEnabler(QCheckBox* checkBox, QString config)
{
    Config()->setConfig(config, checkBox->isChecked());
    triggerAsmOptionsChanged();
}
