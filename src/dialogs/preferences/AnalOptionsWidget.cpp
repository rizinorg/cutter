#include "AnalOptionsWidget.h"
#include "ui_AnalOptionsWidget.h"

#include "PreferencesDialog.h"

#include "common/Helpers.h"
#include "common/Configuration.h"

#include "core/MainWindow.h"

static const QHash<QString, QString> analBoundaries {
    { "io.maps.x", "All executable maps" },
    { "io.maps", "All maps" },
    { "io.map", "Current map" },
    { "raw", "Raw" },
    { "bin.section", "Current mapped section" },
    { "bin.sections", "All mapped sections" },
};

AnalOptionsWidget::AnalOptionsWidget(PreferencesDialog *dialog)
    : QDialog(dialog), ui(new Ui::AnalOptionsWidget)
{
    ui->setupUi(this);

    checkboxes = { { ui->autonameCheckbox, "analysis.autoname" },
                   { ui->hasnextCheckbox, "analysis.hasnext" },
                   { ui->jmpRefCheckbox, "analysis.jmp.ref" },
                   { ui->jmpTblCheckbox, "analysis.jmp.tbl" },
                   { ui->pushRetCheckBox, "analysis.pushret" },
                   { ui->typesVerboseCheckBox, "analysis.types.verbose" },
                   { ui->verboseCheckBox, "analysis.verbose" } };

    // Create list of options for the analysis.in selector
    createAnalInOptionsList();

    // Connect each checkbox from "checkboxes" to the generic signal "checkboxEnabler"
    for (ConfigCheckbox &confCheckbox : checkboxes) {
        QString val = confCheckbox.config;
        QCheckBox &cb = *confCheckbox.checkBox;
        connect(confCheckbox.checkBox, &QCheckBox::stateChanged, this,
                [this, val, &cb]() { checkboxEnabler(&cb, val); });
    }

    ui->analyzePushButton->setToolTip("Analyze the program using Rizin's \"aaa\" command");
    auto *mainWindow = new MainWindow(this);
    connect(ui->analyzePushButton, &QPushButton::clicked, mainWindow,
            &MainWindow::on_actionAnalyze_triggered);
    connect<void (QComboBox::*)(int)>(ui->analInComboBox, &QComboBox::currentIndexChanged, this,
                                      &AnalOptionsWidget::updateAnalIn);
    connect<void (QSpinBox::*)(int)>(ui->ptrDepthSpinBox, &QSpinBox::valueChanged, this,
                                     &AnalOptionsWidget::updateAnalPtrDepth);
    connect(ui->preludeLineEdit, &QLineEdit::textChanged, this,
            &AnalOptionsWidget::updateAnalPrelude);
    updateAnalOptionsFromVars();
}

AnalOptionsWidget::~AnalOptionsWidget() {}

void AnalOptionsWidget::checkboxEnabler(QCheckBox *checkBox, const QString &config)
{
    Config()->setConfig(config, checkBox->isChecked());
}

void AnalOptionsWidget::updateAnalOptionsFromVars()
{
    for (ConfigCheckbox &confCheckbox : checkboxes) {
        qhelpers::setCheckedWithoutSignals(confCheckbox.checkBox,
                                           Core()->getConfigb(confCheckbox.config));
    }

    // Update the rest of analysis options that are not checkboxes
    ui->analInComboBox->setCurrentIndex(
            ui->analInComboBox->findData(Core()->getConfig("analysis.in")));
    ui->ptrDepthSpinBox->setValue(Core()->getConfigi("analysis.ptrdepth"));
    ui->preludeLineEdit->setText(Core()->getConfig("analysis.prelude"));
}

void AnalOptionsWidget::updateAnalIn(int index)
{
    Config()->setConfig("analysis.in", ui->analInComboBox->itemData(index).toString());
}

void AnalOptionsWidget::updateAnalPtrDepth(int value)
{
    Config()->setConfig("analysis.ptrdepth", value);
}

void AnalOptionsWidget::updateAnalPrelude(const QString &prelude)
{
    Config()->setConfig("analysis.prelude", prelude);
}

void AnalOptionsWidget::createAnalInOptionsList()
{
    QHash<QString, QString>::const_iterator mapIter;

    mapIter = analBoundaries.cbegin();
    ui->analInComboBox->blockSignals(true);
    ui->analInComboBox->clear();
    for (; mapIter != analBoundaries.cend(); ++mapIter) {
        ui->analInComboBox->addItem(mapIter.value(), mapIter.key());
    }
    ui->analInComboBox->blockSignals(false);
}
