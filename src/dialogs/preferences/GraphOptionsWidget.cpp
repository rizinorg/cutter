#include <QLabel>
#include <QFontDialog>

#include "GraphOptionsWidget.h"
#include "ui_GraphOptionsWidget.h"

#include "PreferencesDialog.h"

#include "common/Helpers.h"
#include "common/Configuration.h"

GraphOptionsWidget::GraphOptionsWidget(PreferencesDialog *dialog)
    : QDialog(dialog),
      ui(new Ui::GraphOptionsWidget)
{
    ui->setupUi(this);
    ui->checkTransparent->setChecked(Config()->getBitmapTransparentState());
    ui->bitmapGraphScale->setValue(Config()->getBitmapExportScaleFactor()*100.0);
    updateOptionsFromVars();

    connect(Core(), SIGNAL(graphOptionsChanged()), this, SLOT(updateOptionsFromVars()));
}

GraphOptionsWidget::~GraphOptionsWidget() {}


void GraphOptionsWidget::updateOptionsFromVars()
{
    qhelpers::setCheckedWithoutSignals(ui->graphOffsetCheckBox, Config()->getConfigBool("graph.offset"));
    ui->maxColsSpinBox->blockSignals(true);
    ui->maxColsSpinBox->setValue(Config()->getGraphBlockMaxChars());
    ui->maxColsSpinBox->blockSignals(false);
}


void GraphOptionsWidget::triggerOptionsChanged()
{
    disconnect(Core(), SIGNAL(graphOptionsChanged()), this, SLOT(updateOptionsFromVars()));
    Core()->triggerGraphOptionsChanged();
    connect(Core(), SIGNAL(graphOptionsChanged()), this, SLOT(updateOptionsFromVars()));
}

void GraphOptionsWidget::on_maxColsSpinBox_valueChanged(int value)
{
    Config()->setGraphBlockMaxChars(value);
    triggerOptionsChanged();
}

void GraphOptionsWidget::on_graphOffsetCheckBox_toggled(bool checked)
{
    Config()->setConfig("graph.offset", checked);
    triggerOptionsChanged();
}

void GraphOptionsWidget::on_checkTransparent_stateChanged(int checked)
{
    bool temp_transparency;
    if(checked){
        temp_transparency=true;
    }else{
        temp_transparency=false;
    }
    Config()->setBitmapTransparentState(checked);

}

void GraphOptionsWidget::on_bitmapGraphScale_valueChanged(double value)
{
    double value_decimal = value/(double)100.0;
    Config()->setBitmapExportScaleFactor(value_decimal);
}

