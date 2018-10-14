#include <QLabel>
#include <QFontDialog>

#include "GraphOptionsWidget.h"
#include "ui_GraphOptionsWidget.h"

#include "PreferencesDialog.h"

#include "utils/Helpers.h"
#include "utils/Configuration.h"

GraphOptionsWidget::GraphOptionsWidget(PreferencesDialog */*dialog*/, QWidget *parent)
    : AbstractOptionWidget(parent),
      ui(new Ui::GraphOptionsWidget)
{
    ui->setupUi(this);

    updateOptionsFromVars();

    connect(Core(), SIGNAL(graphOptionsChanged()), this, SLOT(updateOptionsFromVars()));
}

GraphOptionsWidget::~GraphOptionsWidget() {}

void GraphOptionsWidget::apply()
{
    Config()->setGraphBlockMaxChars(currSettings.value("graphBlockMaxChars").toInt());
    Config()->setConfig("graph.offset", currSettings.value("graph.offset").toBool());
    isChanged = false;
}

void GraphOptionsWidget::discard()
{
    updateOptionsFromVars();
    isChanged = false;
}

void GraphOptionsWidget::updateOptionsFromVars()
{
    qhelpers::setCheckedWithoutSignals(ui->graphOffsetCheckBox, Config()->getConfigBool("graph.offset"));
    currSettings.setValue("graph.offset", Config()->getConfigBool("graph.offset"));
    ui->maxColsSpinBox->blockSignals(true);
    ui->maxColsSpinBox->setValue(Config()->getGraphBlockMaxChars());
    currSettings.setValue("graphBlockMaxChars", Config()->getGraphBlockMaxChars());
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
    currSettings.setValue("graphBlockMaxChars", value);
    isChanged = true;
}

void GraphOptionsWidget::on_graphOffsetCheckBox_toggled(bool checked)
{
    currSettings.setValue("graph.offset", checked);
    isChanged = true;
}
