#include "InterfaceOptionsWidget.h"
#include "ui_InterfaceOptionsWidget.h"

#include "PreferencesDialog.h"
#include "Configuration.h"

InterfaceOptionsWidget::InterfaceOptionsWidget(PreferencesDialog *dialog)
    : QDialog(dialog), ui(new Ui::InterfaceOptionsWidget)
{
    ui->setupUi(this);

    setUpQuickFilter();
    setUpFunctions();
}

InterfaceOptionsWidget::~InterfaceOptionsWidget() {}

void InterfaceOptionsWidget::setUpFunctions()
{
    bool truncate = Config()->getTruncateFunctionNameCol();
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
