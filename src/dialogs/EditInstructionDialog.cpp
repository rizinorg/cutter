#include "EditInstructionDialog.h"
#include "ui_EditInstructionDialog.h"
#include "core/Cutter.h"

EditInstructionDialog::EditInstructionDialog(InstructionEditMode editMode, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditInstructionDialog),
    editMode(editMode)
{
    ui->setupUi(this);
    ui->lineEdit->setMinimumWidth(400);
    ui->instructionLabel->setWordWrap(true);
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

QString EditInstructionDialog::getInstruction() const
{
    return ui->lineEdit->text();
}

void EditInstructionDialog::setInstruction(const QString &instruction)
{
    ui->lineEdit->setText(instruction);
    ui->lineEdit->selectAll();
    updatePreview(instruction);
}

void EditInstructionDialog::updatePreview(const QString &input)
{
    QString result;

    if (editMode == EDIT_NONE) {
        ui->instructionLabel->setText("");
        return;
    } else if (editMode == EDIT_BYTES) {
        QByteArray data = CutterCore::hexStringToBytes(input);
        result = Core()->disassemble(data).replace('\n', "; ");
    } else if (editMode == EDIT_TEXT) {
        QByteArray data = Core()->assemble(input);
        result = CutterCore::bytesToHexString(data).trimmed();
    }

    if (result.isEmpty() || result.contains("invalid")) {
        ui->instructionLabel->setText("Unknown Instruction");
    } else {
        ui->instructionLabel->setText(result);
    }
}
