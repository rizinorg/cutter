#include "EditInstructionDialog.h"
#include "ui_EditInstructionDialog.h"
#include "core/Cutter.h"

#include <QCheckBox>

EditInstructionDialog::EditInstructionDialog(InstructionEditMode editMode, QWidget *parent)
    : QDialog(parent), ui(new Ui::EditInstructionDialog), editMode(editMode)
{
    ui->setupUi(this);
    ui->lineEdit->setMinimumWidth(400);
    ui->instructionLabel->setWordWrap(true);
    if (editMode == EDIT_TEXT) {
        ui->fillWithNops->setVisible(true);
        ui->fillWithNops->setCheckState(Qt::Checked);
    } else {
        ui->fillWithNops->setVisible(false);
    }
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    connect(ui->lineEdit, &QLineEdit::textEdited, this, &EditInstructionDialog::updatePreview);
}

EditInstructionDialog::~EditInstructionDialog() {}

void EditInstructionDialog::on_buttonBox_accepted() {}

void EditInstructionDialog::on_buttonBox_rejected()
{
    close();
}

bool EditInstructionDialog::needsNops() const
{
    if (editMode != EDIT_TEXT) {
        return false;
    }

    return ui->fillWithNops->checkState() == Qt::Checked;
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
