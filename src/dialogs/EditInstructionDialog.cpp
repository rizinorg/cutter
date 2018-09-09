#include "EditInstructionDialog.h"
#include "ui_EditInstructionDialog.h"
#include "Cutter.h"

EditInstructionDialog::EditInstructionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditInstructionDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    // Event filter for capturing Ctrl/Cmd+Return
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

void EditInstructionDialog::updatePreview(const QString &hex) {
    QString result = Core()->disassemble(hex).trimmed();
    if (result.isEmpty() || result.contains("\n")) {
        ui->instructionLabel->setText("Unknown Instruction");
    } else {
        ui->instructionLabel->setText(result);
    }
}

bool EditInstructionDialog::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);

    if (event -> type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast <QKeyEvent *>(event);

        // Confirm comment by pressing Ctrl/Cmd+Return
        if ((keyEvent -> modifiers() & Qt::ControlModifier) &&
                ((keyEvent -> key() == Qt::Key_Enter) || (keyEvent -> key() == Qt::Key_Return))) {
            this->accept();
            return true;
        }
        
        // Update instruction preview
        QString lineText = ui->lineEdit->text();
        if (keyEvent -> key() == Qt::Key_Backspace) {
            updatePreview(lineText.left(lineText.size() - 1));
        } else {
            updatePreview(lineText + keyEvent->text());
        }
    }


    return false;
}
