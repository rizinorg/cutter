#include "InterfaceOptionsWidget.h"

#include "Configuration.h"
#include "PreferencesDialog.h"
#include "ui_InterfaceOptionsWidget.h"

InterfaceOptionsWidget::InterfaceOptionsWidget(PreferencesDialog *dialog)
    : QDialog(dialog), ui(new Ui::InterfaceOptionsWidget)
{
    ui->setupUi(this);

    setUpQuickFilter();
    setUpOmnibar();
    setUpFunctions();
}

InterfaceOptionsWidget::~InterfaceOptionsWidget() {}

void InterfaceOptionsWidget::setUpFunctions()
{
    const bool truncate = Config()->getTruncateFunctionNameCol();
    ui->fcnTruncateCheckBox->setChecked(truncate);
    ui->fcnTruncateSpinBox->setEnabled(truncate);
    ui->fcnTruncateSpinBox->setValue(Config()->getFunctionNameColWidth());

    connect(ui->fcnTruncateCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        ui->fcnTruncateSpinBox->setEnabled(checked);
        Config()->setTruncateFunctionNameCol(checked);
    });

    connect<void (QSpinBox::*)(int)>(ui->fcnTruncateSpinBox, &QSpinBox::valueChanged, Config(),
                                     &Configuration::setFunctionNameColWidth);
}

void InterfaceOptionsWidget::setUpOmnibar()
{
    const bool limitEntries = Config()->getOmnibarLimitEntries();
    ui->omnibarCountCheckBox->setChecked(limitEntries);
    ui->omnibarCountSpinBox->setValue(Config()->getOmnibarEntriesCount());

    auto toggleWidgets = [this](bool enable) {
        ui->omnibarCountSpinBox->setEnabled(enable);
        ui->omnibarIncrementLabel->setEnabled(enable);
        ui->omnibarIncrementSpinBox->setEnabled(enable);
    };
    toggleWidgets(limitEntries);
    ui->omnibarIncrementSpinBox->setValue(Config()->getOmnibarEntriesIncrement());

    connect(ui->omnibarCountCheckBox, &QCheckBox::toggled, this, [toggleWidgets](bool checked) {
        Config()->setOmnibarLimitEntries(checked);
        toggleWidgets(checked);
    });
    connect<void (QSpinBox::*)(int)>(ui->omnibarCountSpinBox, &QSpinBox::valueChanged, Config(),
                                     &Configuration::setOmnibarEntriesCount);
    connect<void (QSpinBox::*)(int)>(ui->omnibarIncrementSpinBox, &QSpinBox::valueChanged, Config(),
                                     &Configuration::setOmnibarEntriesIncrement);
}

void InterfaceOptionsWidget::setUpQuickFilter()
{
    ui->quickFilterCheckBox->setChecked(Config()->getShowQuickFilter());
    connect(ui->quickFilterCheckBox, &QCheckBox::toggled, Config(),
            &Configuration::setShowQuickFilter);

    ui->itemCountCheckBox->setChecked(Config()->getItemCountVisible());
    connect(ui->itemCountCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        Config()->setItemCountVisible(checked);
        ui->hideItemCountCheckBox->setEnabled(checked);
    });

    connect(ui->hideItemCountCheckBox, &QCheckBox::toggled, Config(),
            &Configuration::setItemCountAutoHide);
    ui->hideItemCountCheckBox->setChecked(Config()->getItemCountAutoHide());
    ui->hideItemCountCheckBox->setEnabled(ui->itemCountCheckBox->isChecked());
}
