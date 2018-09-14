#include "EditInstructionDialog.h"
#include "ui_EditInstructionDialog.h"
#include "Cutter.h"

EditInstructionDialog::EditInstructionDialog(QWidget *parent, bool isEditingBytes) :
    QDialog(parent),
    ui(new Ui::EditInstructionDialog),
    isEditingBytes(isEditingBytes)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    ui->lineEdit->installEventFilter(this);
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
    if (isEditingBytes) {
        result = Core()->disassemble(input).trimmed();
    } else {
        result = Core()->assemble(input).trimmed();
    }

    if (result.isEmpty() || result.contains("\n")) {
        ui->instructionLabel->setText("Unknown Instruction");
    } else {
        ui->instructionLabel->setText(result);
    }
}

bool EditInstructionDialog::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);

    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast <QKeyEvent *>(event);

        // Update instruction preview
        QString lineText = ui->lineEdit->text();
        if (keyEvent->key() == Qt::Key_Backspace) {
            updatePreview(lineText.left(lineText.size() - 1));
        } else {
            updatePreview(lineText + keyEvent->text());
        }
    }

    return false;
}