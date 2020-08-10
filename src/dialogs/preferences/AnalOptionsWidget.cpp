#include "AnalOptionsWidget.h"
#include "ui_AnalOptionsWidget.h"

#include "PreferencesDialog.h"

#include "common/Helpers.h"
#include "common/Configuration.h"

#include "core/MainWindow.h"

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

    // Connect each checkbox from "checkboxes" to the generic signal "checkboxEnabler"
    for (ConfigCheckbox &confCheckbox : checkboxes) {
        QString val = confCheckbox.config;
        QCheckBox &cb = *confCheckbox.checkBox;
        connect(confCheckbox.checkBox, &QCheckBox::stateChanged, this, [this, val, &cb]() { checkboxEnabler(&cb, val); });
    }

    ui->analyzePushButton->setToolTip("Analyze the program using radare2's \"aaa\" command");
    auto *mainWindow = new MainWindow(this);
    connect(ui->analyzePushButton, &QPushButton::clicked, mainWindow, &MainWindow::on_actionAnalyze_triggered);
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
}