#include "AnalOptionsWidget.h"
#include "ui_AnalOptionsWidget.h"

#include "PreferencesDialog.h"

#include "common/Helpers.h"
#include "common/Configuration.h"

#include "core/MainWindow.h"

static const QHash<QString, QString> analBoundaries {
    {"io.maps.x", "All executable maps"},
    {"io.maps", "All maps"},
    {"io.map", "Current map"},
    {"raw", "Raw"},
    {"bin.section", "Current mapped section"},
    {"bin.sections", "All mapped sections"},
};

AnalOptionsWidget::AnalOptionsWidget(PreferencesDialog *dialog)
    : QDialog(dialog),
      ui(new Ui::AnalOptionsWidget)
{
    ui->setupUi(this);

    checkboxes = {
        { ui->autonameCheckbox,     "anal.autoname" },
        { ui->hasnextCheckbox,      "anal.hasnext" },
        { ui->jmpRefCheckbox,       "anal.jmp.ref" },
        { ui->jmpTblCheckbox,       "anal.jmp.tbl" },
        { ui->pushRetCheckBox,      "anal.pushret" },
        { ui->typesVerboseCheckBox, "anal.types.verbose" },
        { ui->verboseCheckBox,      "anal.verbose" }
    };

    // Create list of options for the anal.in selector
    createAnalInOptionsList();

    // Connect each checkbox from "checkboxes" to the generic signal "checkboxEnabler"
    for (ConfigCheckbox &confCheckbox : checkboxes) {
        QString val = confCheckbox.config;
        QCheckBox &cb = *confCheckbox.checkBox;
        connect(confCheckbox.checkBox, &QCheckBox::stateChanged, this, [this, val, &cb]() { checkboxEnabler(&cb, val); });
    }

    ui->analyzePushButton->setToolTip("Analyze the program using radare2's \"aaa\" command");
    auto *mainWindow = new MainWindow(this);
    connect(ui->analyzePushButton, &QPushButton::clicked, mainWindow,
            &MainWindow::on_actionAnalyze_triggered);
    connect<void (QComboBox::*)(int)>(ui->analInComboBox, &QComboBox::currentIndexChanged, this,
                                      &AnalOptionsWidget::updateAnalIn);
    connect<void (QSpinBox::*)(int)>(ui->ptrDepthSpinBox, &QSpinBox::valueChanged, this,
                                     &AnalOptionsWidget::updateAnalPtrDepth);
    connect(ui->preludeLineEdit, &QLineEdit::textChanged, this, &AnalOptionsWidget::updateAnalPrelude);
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
        qhelpers::setCheckedWithoutSignals(confCheckbox.checkBox, Core()->getConfigb(confCheckbox.config));
    }

    // Update the rest of analysis options that are not checkboxes
    ui->analInComboBox->setCurrentIndex(ui->analInComboBox->findData(Core()->getConfig("anal.in")));
    ui->ptrDepthSpinBox->setValue(Core()->getConfigi("anal.ptrdepth"));
    ui->preludeLineEdit->setText(Core()->getConfig("anal.prelude"));
}

void AnalOptionsWidget::updateAnalIn(int index)
{
    Config()->setConfig("anal.in", ui->analInComboBox->itemData(index).toString());
}

void AnalOptionsWidget::updateAnalPtrDepth(int value)
{
    Config()->setConfig("anal.ptrdepth", value);
}

void AnalOptionsWidget::updateAnalPrelude(const QString &prelude)
{
    Config()->setConfig("anal.prelude", prelude);
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
