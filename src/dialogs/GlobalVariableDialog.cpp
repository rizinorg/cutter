#include "GlobalVariableDialog.h"
#include "ui_GlobalVariableDialog.h"

#include <QIntValidator>
#include "core/Cutter.h"

GlobalVariableDialog::GlobalVariableDialog(RVA offset, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::GlobalVariableDialog),
      offset(offset),
      globalVariableName(""),
      globalVariableOffset(RVA_INVALID)
{
    // Setup UI
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
    RzCoreLocked core = Core()->lock();
    RzAnalysisVarGlobal *globalVariable =
            rz_analysis_var_global_get_byaddr_at(core->analysis, offset);
    if (globalVariable) {
        globalVariableName = QString(globalVariable->name);
        globalVariableOffset = globalVariable->addr;
    }

    if (globalVariable) {
        ui->nameEdit->setText(globalVariable->name);
        QString globalVarType = Core()->getGlobalVariableType(globalVariable->name);
        ui->typeEdit->setText(globalVarType);
        ui->labelAction->setText(tr("Edit global variable at %1").arg(RzAddressString(offset)));
    } else {
        ui->labelAction->setText(tr("Add global variable at %1").arg(RzAddressString(offset)));
    }

    // Connect slots
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
            &GlobalVariableDialog::buttonBoxAccepted);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
            &GlobalVariableDialog::buttonBoxRejected);
}

GlobalVariableDialog::~GlobalVariableDialog() {}

void GlobalVariableDialog::buttonBoxAccepted()
{
    QString name = ui->nameEdit->text();
    QString typ = ui->typeEdit->text();

    if (name.isEmpty()) {
        if (globalVariableOffset != RVA_INVALID) {
            // Empty name and global variable exists -> delete the global variable
            Core()->delGlobalVariable(globalVariableOffset);
        } else {
            // GlobalVariable was not existing and we gave an empty name, do nothing
        }
    } else {
        if (globalVariableOffset != RVA_INVALID) {
            // Name provided and global variable exists -> rename the global variable
            Core()->modifyGlobalVariable(globalVariableOffset, name, typ);
        } else {
            // Name provided and global variable does not exist -> create the global variable
            Core()->addGlobalVariable(offset, name, typ);
        }
    }
    close();
    this->setResult(QDialog::Accepted);
}

void GlobalVariableDialog::buttonBoxRejected()
{
    close();
    this->setResult(QDialog::Rejected);
}
