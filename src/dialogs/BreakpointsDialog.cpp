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

    ui->positionType->setItemData(0, BreakpointDescription::Address);
    ui->positionType->setItemData(1, BreakpointDescription::Named);
    ui->positionType->setItemData(2, BreakpointDescription::Module);

    connect(ui->positionType, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &BreakpointsDialog::onTypeChanged);
    onTypeChanged();
}

BreakpointsDialog::BreakpointsDialog(const BreakpointDescription &breakpoint, QWidget *parent)
    : BreakpointsDialog(true, parent)
{
    switch (breakpoint.type) {
    case BreakpointDescription::Address:
        ui->breakpointPosition->setText(RAddressString(breakpoint.addr));
        break;
    case BreakpointDescription::Named:
        ui->breakpointPosition->setText(breakpoint.positionExpression);
        break;
    case BreakpointDescription::Module:
        ui->breakpointPosition->setText(QString::number(breakpoint.moduleDelta));
        ui->moduleName->setText(breakpoint.positionExpression);
        break;
    }
    for (int i = 0; i < ui->positionType->count(); i++) {
        if (ui->positionType->itemData(i) == breakpoint.type) {
            ui->positionType->setCurrentIndex(i);
        }
    }
    ui->breakpointCommand->setPlainText(breakpoint.command);
    ui->breakpointCondition->setEditText(breakpoint.condition);
    if (breakpoint.hw) {
        ui->radioHardware->setChecked(true);
        ui->hwPermissions->setCurrentText(breakpoint.permission);
        ui->breakpointSize->setCurrentText(QString::number(breakpoint.size));
    } else {
        ui->radioSoftware->setChecked(true);
    }
    ui->checkTrace->setChecked(breakpoint.trace);
    ui->checkEnabled->setChecked(breakpoint.enabled);
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
    auto positionType = ui->positionType->currentData().value<BreakpointDescription::PositionType>();
    switch (positionType) {
    case BreakpointDescription::Address:
        breakpoint.addr = Core()->math(ui->breakpointPosition->text());
        break;
    case BreakpointDescription::Named:
        breakpoint.positionExpression = ui->breakpointPosition->text().trimmed();
        break;
    case BreakpointDescription::Module:
        breakpoint.moduleDelta = static_cast<int64_t>(Core()->math(ui->breakpointPosition->text()));
        breakpoint.positionExpression = ui->moduleName->text().trimmed();
        break;
    }
    breakpoint.type = positionType;

    breakpoint.size = Core()->num(ui->breakpointSize->currentText());
    breakpoint.condition = ui->breakpointCondition->currentText().trimmed();
    breakpoint.command = ui->breakpointCommand->toPlainText().trimmed();
    if (ui->radioHardware->isChecked()) {
        breakpoint.hw = true;
        breakpoint.permission = ui->hwPermissions->currentText();
    } else {
        breakpoint.hw = false;
    }
    breakpoint.trace = ui->checkTrace->isChecked();
    breakpoint.enabled = ui->checkEnabled->isChecked();
    return breakpoint;
}

void BreakpointsDialog::createNewBreakpoint(RVA address, QWidget *parent)
{
    BreakpointsDialog editDialog(address, parent);
    if (editDialog.exec() == QDialog::Accepted) {
        Core()->addBreakpoint(editDialog.getDescription());
    }
}

void BreakpointsDialog::editBreakpoint(const BreakpointDescription &breakpoint, QWidget *parent)
{
    BreakpointsDialog editDialog(breakpoint, parent);
    if (editDialog.exec() == QDialog::Accepted) {
        Core()->updateBreakpoint(breakpoint.index, editDialog.getDescription());
    }
}

void BreakpointsDialog::refreshOkButton()
{
    auto button = ui->buttonBox->button(QDialogButtonBox::StandardButton::Ok);
    button->setDisabled(ui->breakpointPosition->text().isEmpty());
}

void BreakpointsDialog::onTypeChanged()
{
    bool moduleEnabled = ui->positionType->currentData() == QVariant(BreakpointDescription::Module);
    ui->moduleLabel->setEnabled(moduleEnabled);
    ui->moduleName->setEnabled(moduleEnabled);
}
