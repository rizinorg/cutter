#include "InterfaceOptionsWidget.h"
#include "ui_InterfaceOptionsWidget.h"

#include "PreferencesDialog.h"
#include "Configuration.h"

InterfaceOptionsWidget::InterfaceOptionsWidget(PreferencesDialog *dialog)
    : QDialog(dialog), ui(new Ui::InterfaceOptionsWidget)
{
    ui->setupUi(this);

    setUpQuickFilter();
}

InterfaceOptionsWidget::~InterfaceOptionsWidget() {}

void InterfaceOptionsWidget::setUpQuickFilter()
{
    ui->quickFilterCheckBox->setChecked(Config()->getShowQuickFilter());
    connect(ui->quickFilterCheckBox, &QCheckBox::toggled, this,
            [](bool checked) { Config()->setShowQuickFilter(checked); });

    ui->itemCountCheckBox->setChecked(Config()->getItemCountVisible());
    connect(ui->itemCountCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        Config()->setItemCountVisible(checked);
        ui->hideItemCountCheckBox->setEnabled(checked);
    });

    connect(ui->hideItemCountCheckBox, &QCheckBox::toggled, this,
            [](bool checked) { Config()->setItemCountAutoHide(checked); });
    ui->hideItemCountCheckBox->setChecked(Config()->getItemCountAutoHide());
    ui->hideItemCountCheckBox->setEnabled(ui->itemCountCheckBox->isChecked());
}
