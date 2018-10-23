#include "EditInstructionDialog.h"
#include "ui_EditInstructionDialog.h"
#include "Cutter.h"

EditInstructionDialog::EditInstructionDialog(QWidget *parent, InstructionEditMode editMode) :
    QDialog(parent),
    ui(new Ui::EditInstructionDialog),
    editMode(editMode)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    connect(ui->lineEdit, SIGNAL(textEdited(const QString &)), this,
            SLOT(updatePreview(const QString &)));
}

EditInstructionDialog::~EditInstructionDialog() {}

void EditInstructionDialog::on_buttonBox_accepted()
{
}

void EditInstructionDialog::on_buttonBox_rejected()
{
    close();
}

QString EditInstructionDialog::getInstruction()
{
    QString ret = ui->lineEdit->text();
    return ret;
}

void EditInstructionDialog::setInstruction(const QString &instruction)
{
    ui->lineEdit->setText(instruction);
    updatePreview(instruction);
}

void EditInstructionDialog::updatePreview(const QString &input)
{
    QString result;

    if (editMode == EDIT_NONE) {
        ui->instructionLabel->setText("");
        return;
    } else if (editMode == EDIT_BYTES) {
        result = Core()->disassemble(input).trimmed();
    } else if (editMode == EDIT_TEXT) {
        result = Core()->assemble(input).trimmed();
    }

    if (result.isEmpty() || result.contains("\n")) {
        ui->instructionLabel->setText("Unknown Instruction");
    } else {
        ui->instructionLabel->setText(result);
    }
}
