#include "BreakpointsDialog.h"
#include "ui_BreakpointsDialog.h"

BreakpointsDialog::BreakpointsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BreakpointsDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    // Event filter for capturing Ctrl/Cmd+Return
    ui->textEdit->installEventFilter(this);
}

BreakpointsDialog::~BreakpointsDialog() {}

void BreakpointsDialog::on_buttonBox_accepted()
{
}

void BreakpointsDialog::on_buttonBox_rejected()
{
    close();
}

QString BreakpointsDialog::getBreakpoints()
{
    QString ret = ui->textEdit->document()->toPlainText();
    return ret;
}

bool BreakpointsDialog::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);
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
