#include "BreakpointsDialog.h"
#include "ui_BreakpointsDialog.h"
#include "Cutter.h"
#include "Helpers.h"

#include <QPushButton>
#include <QCompleter>
#include <QCheckBox>

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


    struct {
        QString label;
        QString tooltip;
        BreakpointDescription::PositionType type;
    } positionTypes[] = {
        {tr("Address"), tr("Address or expression calculated when creating breakpoint"), BreakpointDescription::Address},
        {tr("Named"), tr("Expression - stored as expression"), BreakpointDescription::Named},
        {tr("Module offset"), tr("Offset relative to module"), BreakpointDescription::Module},
    };
    int index = 0;
    for (auto &item : positionTypes) {
        ui->positionType->addItem(item.label, item.type);
        ui->positionType->setItemData(index, item.tooltip, Qt::ToolTipRole);
        index++;
    }

    connect(ui->positionType, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &BreakpointsDialog::onTypeChanged);
    onTypeChanged();

    auto modules = Core()->getMemoryMap();
    QSet<QString> moduleNames;
    for (const auto &module : modules) {
        moduleNames.insert(module.fileName);
    }
    for (const auto& module : moduleNames) {
        ui->moduleName->addItem(module);
    }
    ui->moduleName->setCurrentText("");
    // Suggest completion when user tries to enter file name not only full path
    ui->moduleName->completer()->setFilterMode(Qt::MatchContains);

    ui->breakpointCondition->setCompleter(nullptr); // Don't use examples for completing
    configureCheckboxRestrictions();
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
        ui->moduleName->setCurrentText(breakpoint.positionExpression);
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
        ui->hwRead->setChecked(breakpoint.permission & R_BP_PROT_READ);
        ui->hwWrite->setChecked(breakpoint.permission & R_BP_PROT_WRITE);
        ui->hwExecute->setChecked(breakpoint.permission & R_BP_PROT_EXEC);
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
        breakpoint.positionExpression = ui->moduleName->currentText().trimmed();
        break;
    }
    breakpoint.type = positionType;

    breakpoint.size = Core()->num(ui->breakpointSize->currentText());
    breakpoint.condition = ui->breakpointCondition->currentText().trimmed();
    breakpoint.command = ui->breakpointCommand->toPlainText().trimmed();
    if (ui->radioHardware->isChecked()) {
        breakpoint.hw = true;
        breakpoint.permission = getHwPermissions();
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
    ui->breakpointPosition->setPlaceholderText(ui->positionType->currentData(Qt::ToolTipRole).toString());
}

void BreakpointsDialog::configureCheckboxRestrictions()
{
    auto atLeastOneChecked = [this]() {
        if (this->getHwPermissions() == 0) {
            this->ui->hwExecute->setChecked(true);
        }
    };
    auto rwRule = [this, atLeastOneChecked](bool checked) {
        if (checked) {
            this->ui->hwExecute->setChecked(false);
        } else {
            atLeastOneChecked();
        }
    };
    connect(ui->hwRead, &QCheckBox::toggled, this, rwRule);
    connect(ui->hwWrite, &QCheckBox::toggled, this, rwRule);
    auto execRule = [this, atLeastOneChecked](bool checked) {
        if (checked) {
            this->ui->hwRead->setChecked(false);
            this->ui->hwWrite->setChecked(false);
        } else {
            atLeastOneChecked();
        }
    };
    connect(ui->hwExecute, &QCheckBox::toggled, this, execRule);
}

int BreakpointsDialog::getHwPermissions()
{
    int result = 0;
    if (ui->hwRead->isChecked()) {
        result |= R_BP_PROT_READ;
    }
    if (ui->hwWrite->isChecked()) {
        result |= R_BP_PROT_WRITE;
    }
    if (ui->hwExecute->isChecked()) {
        result |= R_BP_PROT_EXEC;
    }
    return result;
}
