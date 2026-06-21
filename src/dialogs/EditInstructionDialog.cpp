#include "EditInstructionDialog.h"

#include "core/Cutter.h"
#include "ui_EditInstructionDialog.h"

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

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
            &EditInstructionDialog::onButtonBoxAccepted);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
            &EditInstructionDialog::onButtonBoxRejected);
}

EditInstructionDialog::~EditInstructionDialog() {}

void EditInstructionDialog::onButtonBoxAccepted() {}

void EditInstructionDialog::onButtonBoxRejected()
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
        const QByteArray data = CutterCore::hexStringToBytes(input);
        result = Core()->disassemble(data).replace('\n', "; ");
    } else if (editMode == EDIT_TEXT) {
        const QByteArray data = Core()->assemble(input);
        result = CutterCore::bytesToHexString(data).trimmed();
    }

    if (result.isEmpty() || result.contains("invalid")) {
        ui->instructionLabel->setText("Unknown Instruction");
    } else {
        ui->instructionLabel->setText(result);
    }
}
