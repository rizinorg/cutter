#include "EditFunctionDialog.h"
#include "ui_EditFunctionDialog.h"

EditFunctionDialog::EditFunctionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditFunctionDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    // Event filter for capturing Ctrl/Cmd+Return
    ui->textEdit->installEventFilter(this);
}

EditFunctionDialog::~EditFunctionDialog() {}

void EditFunctionDialog::on_buttonBox_accepted()
{
}

void EditFunctionDialog::on_buttonBox_rejected()
{
    close();
}

bool EditFunctionDialog::eventFilter(QObject */*obj*/, QEvent *event)
{
    if (event -> type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast <QKeyEvent *> (event);

        // Confirm comment by pressing Ctrl/Cmd+Return
        if ((keyEvent -> modifiers() & Qt::ControlModifier) &&
                ((keyEvent -> key() == Qt::Key_Enter) || (keyEvent -> key() == Qt::Key_Return))) {
            this->accept();
            return true;
        }
    }

    return false;
}
