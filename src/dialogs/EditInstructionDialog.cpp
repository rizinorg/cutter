#include "EditInstructionDialog.h"
#include "ui_EditInstructionDialog.h"

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
    }


    return false;
}
