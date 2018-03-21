#include <QLabel>
#include <QFontDialog>

#include "GraphOptionsWidget.h"
#include "ui_GraphOptionsWidget.h"

#include "PreferencesDialog.h"

#include "utils/Helpers.h"
#include "utils/Configuration.h"

GraphOptionsWidget::GraphOptionsWidget(PreferencesDialog */*dialog*/, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::GraphOptionsWidget)
{
    ui->setupUi(this);

    updateOptionsFromVars();

    connect(Core(), SIGNAL(graphOptionsChanged()), this, SLOT(updateOptionsFromVars()));
}

GraphOptionsWidget::~GraphOptionsWidget() {}


void GraphOptionsWidget::updateOptionsFromVars()
{
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

