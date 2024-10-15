#include <QLabel>
#include <QFontDialog>

#include "GraphOptionsWidget.h"
#include "ui_GraphOptionsWidget.h"

#include "PreferencesDialog.h"

#include "common/Helpers.h"
#include "common/Configuration.h"

GraphOptionsWidget::GraphOptionsWidget(PreferencesDialog *dialog)
    : QDialog(dialog), ui(new Ui::GraphOptionsWidget)
{
    ui->setupUi(this);
    ui->checkTransparent->setChecked(Config()->getBitmapTransparentState());
    ui->blockEntryCheckBox->setChecked(Config()->getGraphBlockEntryOffset());
    ui->graphPreviewCheckBox->setChecked(Config()->getGraphPreview());
    ui->bitmapGraphScale->setValue(Config()->getBitmapExportScaleFactor() * 100.0);
    updateOptionsFromVars();

    connect<void (QDoubleSpinBox::*)(double)>(ui->bitmapGraphScale, (&QDoubleSpinBox::valueChanged),
                                              this,
                                              &GraphOptionsWidget::bitmapGraphScaleValueChanged);
    connect(ui->checkTransparent, &QCheckBox::stateChanged, this,
            &GraphOptionsWidget::checkTransparentStateChanged);
    connect(ui->blockEntryCheckBox, &QCheckBox::stateChanged, this,
            &GraphOptionsWidget::checkGraphBlockEntryOffsetChanged);

    connect(Core(), &CutterCore::graphOptionsChanged, this,
            &GraphOptionsWidget::updateOptionsFromVars);
    QSpinBox *graphSpacingWidgets[] = { ui->horizontalEdgeSpacing, ui->horizontalBlockSpacing,
                                        ui->verticalEdgeSpacing, ui->verticalBlockSpacing };
    for (auto widget : graphSpacingWidgets) {
        connect<void (QSpinBox::*)(int)>(widget, &QSpinBox::valueChanged, this,
                                         &GraphOptionsWidget::layoutSpacingChanged);
    }
}

GraphOptionsWidget::~GraphOptionsWidget() {}

void GraphOptionsWidget::updateOptionsFromVars()
{
    qhelpers::setCheckedWithoutSignals(ui->graphOffsetCheckBox,
                                       Config()->getConfigBool("asm.offset"));
    ui->maxColsSpinBox->blockSignals(true);
    ui->maxColsSpinBox->setValue(Config()->getGraphBlockMaxChars());
    ui->maxColsSpinBox->blockSignals(false);
    ui->minFontSizeSpinBox->blockSignals(true);
    ui->minFontSizeSpinBox->setValue(Config()->getGraphMinFontSize());
    ui->minFontSizeSpinBox->blockSignals(false);
    auto blockSpacing = Config()->getGraphBlockSpacing();
    ui->horizontalBlockSpacing->setValue(blockSpacing.x());
    ui->verticalBlockSpacing->setValue(blockSpacing.y());
    auto edgeSpacing = Config()->getGraphEdgeSpacing();
    ui->horizontalEdgeSpacing->setValue(edgeSpacing.x());
    ui->verticalEdgeSpacing->setValue(edgeSpacing.y());
}

void GraphOptionsWidget::triggerOptionsChanged()
{
    disconnect(Core(), &CutterCore::graphOptionsChanged, this,
               &GraphOptionsWidget::updateOptionsFromVars);
    Core()->triggerGraphOptionsChanged();
    connect(Core(), &CutterCore::graphOptionsChanged, this,
            &GraphOptionsWidget::updateOptionsFromVars);
}

void GraphOptionsWidget::on_maxColsSpinBox_valueChanged(int value)
{
    Config()->setGraphBlockMaxChars(value);
    triggerOptionsChanged();
}

void GraphOptionsWidget::on_minFontSizeSpinBox_valueChanged(int value)
{
    Config()->setGraphMinFontSize(value);
    triggerOptionsChanged();
}

void GraphOptionsWidget::on_graphOffsetCheckBox_toggled(bool checked)
{
    Config()->setConfig("asm.offset", checked);
    Core()->triggerAsmOptionsChanged();
    triggerOptionsChanged();
}

void GraphOptionsWidget::on_graphPreviewCheckBox_toggled(bool checked)
{
    Config()->setGraphPreview(checked);
    triggerOptionsChanged();
}

void GraphOptionsWidget::checkTransparentStateChanged(int checked)
{
    Config()->setBitmapTransparentState(checked);
}

void GraphOptionsWidget::bitmapGraphScaleValueChanged(double value)
{
    double value_decimal = value / (double)100.0;
    Config()->setBitmapExportScaleFactor(value_decimal);
}

void GraphOptionsWidget::layoutSpacingChanged()
{
    QPoint blockSpacing { ui->horizontalBlockSpacing->value(), ui->verticalBlockSpacing->value() };
    QPoint edgeSpacing { ui->horizontalEdgeSpacing->value(), ui->verticalEdgeSpacing->value() };
    Config()->setGraphSpacing(blockSpacing, edgeSpacing);
    triggerOptionsChanged();
}

void GraphOptionsWidget::checkGraphBlockEntryOffsetChanged(bool checked)
{
    Config()->setGraphBlockEntryOffset(checked);
    triggerOptionsChanged();
}
