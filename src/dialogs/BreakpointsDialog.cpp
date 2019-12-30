#include "BreakpointsDialog.h"
#include "ui_BreakpointsDialog.h"
#include "Cutter.h"

#include <QPushButton>

BreakpointsDialog::BreakpointsDialog(bool editMode, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BreakpointsDialog),
    editMode(editMode)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    connect(ui->breakpointPosition, &QLineEdit::textChanged, this, &BreakpointsDialog::refreshOkButton);
    refreshOkButton();

    if (editMode) {
        setWindowTitle(tr("Edit breakpoint"));
    } else {
        setWindowTitle(tr("New breakpoint"));
    }
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
    refreshOkButton();
}

BreakpointsDialog::BreakpointsDialog(RVA address, QWidget *parent)
    : BreakpointsDialog(false, parent)
{
    if (address != RVA_INVALID) {
        ui->breakpointPosition->setText(RAddressString(address));
    }
    refreshOkButton();
}

BreakpointsDialog::~BreakpointsDialog() {}

BreakpointDescription BreakpointsDialog::getDescription()
{
    BreakpointDescription breakpoint;
    breakpoint.addr = Core()->math(ui->breakpointPosition->text());
    breakpoint.size = Core()->num(ui->breakpointSize->currentText());
    breakpoint.condition = ui->breakpointCondition->currentText().trimmed();
    breakpoint.command = ui->breakpointCommand->text().trimmed();
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

void BreakpointsDialog::createNewBreakpoint(RVA address, QWidget *parent)
{
    BreakpointsDialog editDialog(address, parent);
    if (editDialog.exec() == QDialog::Accepted) {
        Core()->addBreakpoint(editDialog.getDescription());
    }
}

void BreakpointsDialog::refreshOkButton()
{
    auto button = ui->buttonBox->button(QDialogButtonBox::StandardButton::Ok);
    button->setDisabled(ui->breakpointPosition->text().isEmpty());
}
