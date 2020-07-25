#include "AnalOptionsWidget.h"
#include "ui_AnalOptionsWidget.h"

#include "common/AsyncTask.h"
#include "common/AnalTask.h"
#include "dialogs/AsyncTaskDialog.h"

#include "PreferencesDialog.h"

#include "common/Helpers.h"
#include "common/Configuration.h"

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

    connect(ui->analyzePushButton, &QPushButton::clicked, this, &AnalOptionsWidget::analyze);
    updateAnalOptionsFromVars();
}

AnalOptionsWidget::~AnalOptionsWidget() {}

/**
 * @brief A generic signal to handle the simple cases where a checkbox is toggled
 * while it's only responsible for a single independent boolean configuration eval.
 * @param checkBox - The checkbox which is responsible for the signal
 * @param config - the configuration string to be toggled
 */
void AnalOptionsWidget::checkboxEnabler(QCheckBox *checkBox, const QString &config)
{
    Config()->setConfig(config, checkBox->isChecked());
}

/**
 * @brief A signal that creates an AsyncTask to re-analyze the current file
 */
void AnalOptionsWidget::analyze()
{
    AnalTask *analTask = new AnalTask();
    InitialOptions options;
    options.analCmd = { {"aaa", "Auto analysis"} };
    analTask->setOptions(options);
    AsyncTask::Ptr analTaskPtr(analTask);

    AsyncTaskDialog *taskDialog = new AsyncTaskDialog(analTaskPtr);
    taskDialog->setInterruptOnClose(true);
    taskDialog->setAttribute(Qt::WA_DeleteOnClose);
    taskDialog->show();
    connect(analTask, &AnalTask::finished, this, &AnalOptionsWidget::refreshAll);

    Core()->getAsyncTaskManager()->start(analTaskPtr);
}

/**
 * @brief A signal to display the options in the dialog according to the current anal.* configuration
 */
void AnalOptionsWidget::updateAnalOptionsFromVars()
{
    for (ConfigCheckbox &confCheckbox : checkboxes) {
        qhelpers::setCheckedWithoutSignals(confCheckbox.checkBox, Core()->getConfigb(confCheckbox.config));
    }
}

/**
 * @brief A signal to refresh all the views after the analysis has finished
 */
void AnalOptionsWidget::refreshAll()
{
    Core()->triggerRefreshAll();
}