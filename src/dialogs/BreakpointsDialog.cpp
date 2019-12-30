#include "BreakpointsDialog.h"
#include "ui_BreakpointsDialog.h"
#include "Cutter.h"

BreakpointsDialog::BreakpointsDialog(bool editMode, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BreakpointsDialog),
    editMode(editMode)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    // Event filter for capturing Ctrl/Cmd+Return
    //ui->textEdit->installEventFilter(this);
}

BreakpointsDialog::BreakpointsDialog(const BreakpointDescription &breakpoint, QWidget *parent)
    : BreakpointsDialog(true, parent)
{
    ui->breakpointPosition->setText(RAddressString(breakpoint.addr));
    ui->breakpointCommand->setText(breakpoint.command);
    ui->breakpointCondition->setEditText(breakpoint.condition);
    if (breakpoint.hw) {
        ui->radioHardware->setChecked(true);
        ui->hwPermissions->setCurrentText(breakpoint.permission);
        ui->breakpointSize->setCurrentText(QString::number(breakpoint.size));
    } else {
        ui->radioSoftware->setChecked(true);
    }
    ui->checkTrace->setChecked(breakpoint.trace);
    ui->checkActive->setChecked(breakpoint.enabled);
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
    QString ret = "";//ui->textEdit->document()->toPlainText();
    return ret;
}

BreakpointDescription BreakpointsDialog::getDescription()
{
    BreakpointDescription breakpoint;
    breakpoint.addr = Core()->num(ui->breakpointPosition->text());
    breakpoint.size = Core()->num(ui->breakpointSize->currentText());
    breakpoint.condition = ui->breakpointCondition->currentText();
    breakpoint.command = ui->breakpointCommand->text();
    if (ui->radioHardware->isChecked()) {
        breakpoint.hw = true;
        breakpoint.permission = ui->hwPermissions->currentText();
    } else {
        breakpoint.hw = false;
    }
    breakpoint.trace = ui->checkTrace->isChecked();
    breakpoint.enabled = ui->checkActive->isChecked();
    return breakpoint;
}

bool BreakpointsDialog::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);
    /*if (event -> type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast <QKeyEvent *> (event);

        // Confirm comment by pressing Ctrl/Cmd+Return
        if ((keyEvent -> modifiers() & Qt::ControlModifier) &&
                ((keyEvent -> key() == Qt::Key_Enter) || (keyEvent -> key() == Qt::Key_Return))) {
            this->accept();
            return true;
        }
    }*/

    return false;
}
