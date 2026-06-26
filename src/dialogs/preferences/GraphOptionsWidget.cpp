#include "GraphOptionsWidget.h"

#include "Cutter.h"
#include "PreferencesDialog.h"
#include "common/Configuration.h"
#include "ui_GraphOptionsWidget.h"

#include <QFontDialog>
#include <QLabel>

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
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    connect(ui->checkTransparent, &QCheckBox::checkStateChanged, this,
            &GraphOptionsWidget::checkTransparentStateChanged);
    connect(ui->blockEntryCheckBox, &QCheckBox::checkStateChanged, this,
            &GraphOptionsWidget::checkGraphBlockEntryOffsetChanged);
#else
    connect(ui->checkTransparent, &QCheckBox::stateChanged, this,
            &GraphOptionsWidget::checkTransparentStateChanged);
    connect(ui->blockEntryCheckBox, &QCheckBox::stateChanged, this,
            &GraphOptionsWidget::checkGraphBlockEntryOffsetChanged);
#endif
    connect(Core(), &CutterCore::graphOptionsChanged, this,
            &GraphOptionsWidget::updateOptionsFromVars);
    const QSpinBox *const graphSpacingWidgets[] = { ui->horizontalEdgeSpacing,
                                                    ui->horizontalBlockSpacing,
                                                    ui->verticalEdgeSpacing,
                                                    ui->verticalBlockSpacing };
    for (auto widget : graphSpacingWidgets) {
        connect<void (QSpinBox::*)(int)>(widget, &QSpinBox::valueChanged, this,
                                         &GraphOptionsWidget::layoutSpacingChanged);
    }

    connect(ui->maxColsSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
            &GraphOptionsWidget::onMaxColsSpinBoxValueChanged);
    connect(ui->minFontSizeSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &GraphOptionsWidget::onMinFontSizeSpinBoxValueChanged);

    connect(ui->graphPreviewCheckBox, &QCheckBox::toggled, this,
            &GraphOptionsWidget::onGraphPreviewCheckBoxToggled);
}

GraphOptionsWidget::~GraphOptionsWidget() {}

void GraphOptionsWidget::updateOptionsFromVars()
{
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

void GraphOptionsWidget::triggerOptionsChanged() const
{
    disconnect(Core(), &CutterCore::graphOptionsChanged, this,
               &GraphOptionsWidget::updateOptionsFromVars);
    Core()->triggerGraphOptionsChanged();
    connect(Core(), &CutterCore::graphOptionsChanged, this,
            &GraphOptionsWidget::updateOptionsFromVars);
}

void GraphOptionsWidget::onMaxColsSpinBoxValueChanged(int value)
{
    Config()->setGraphBlockMaxChars(value);
    triggerOptionsChanged();
}

void GraphOptionsWidget::onMinFontSizeSpinBoxValueChanged(int value)
{
    Config()->setGraphMinFontSize(value);
    triggerOptionsChanged();
}

void GraphOptionsWidget::onGraphPreviewCheckBoxToggled(bool checked)
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
    const double valueDecimal = value / (double)100.0;
    Config()->setBitmapExportScaleFactor(valueDecimal);
}

void GraphOptionsWidget::layoutSpacingChanged()
{
    const QPoint blockSpacing { ui->horizontalBlockSpacing->value(),
                                ui->verticalBlockSpacing->value() };
    const QPoint edgeSpacing { ui->horizontalEdgeSpacing->value(),
                               ui->verticalEdgeSpacing->value() };
    Config()->setGraphSpacing(blockSpacing, edgeSpacing);
    triggerOptionsChanged();
}

void GraphOptionsWidget::checkGraphBlockEntryOffsetChanged(bool checked)
{
    Config()->setGraphBlockEntryOffset(checked);
    triggerOptionsChanged();
}
