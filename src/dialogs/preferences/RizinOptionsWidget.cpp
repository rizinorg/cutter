#include "RizinOptionsWidget.h"
#include "PreferencesDialog.h"
#include "ui_RizinOptionsWidget.h"

#include "common/Helpers.h"
#include "common/Configuration.h"
#include "core/Cutter.h"

RizinOptionsWidget::RizinOptionsWidget(PreferencesDialog *dialog)
    : QDialog(dialog), ui(new Ui::RizinOptionsWidget)
{
    ui->setupUi(this);
    connect(ui->compressProjectCheckBox, &QCheckBox::stateChanged, this,
            &RizinOptionsWidget::updateCompressProjectConfig);

    updateRizinWidgetsFromConfig();
}

RizinOptionsWidget::~RizinOptionsWidget(){}

void RizinOptionsWidget::updateRizinWidgetsFromConfig()
{
    qhelpers::setCheckedWithoutSignals(ui->compressProjectCheckBox,
                                       Core()->getConfigb("prj.compress"));
}

void RizinOptionsWidget::updateCompressProjectConfig()
{
    Config()->setConfig("prj.compress", ui->compressProjectCheckBox->isChecked());
}