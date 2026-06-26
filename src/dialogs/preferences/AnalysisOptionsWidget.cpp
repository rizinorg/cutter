#include "AnalysisOptionsWidget.h"

#include "PreferencesDialog.h"
#include "common/Configuration.h"
#include "common/Helpers.h"
#include "core/MainWindow.h"
#include "ui_AnalysisOptionsWidget.h"

namespace {
const QHash<QString, const char *> analysisBoundaries {
    // Range/Block
    { "range", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "Range") },
    { "block", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "Block") },

    // Bin Segments
    { "bin.segment", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "Bin Segment") },
    { "bin.segments", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "All Bin Segments") },
    { "bin.segments.x", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "Executable Bin Segments") },
    { "bin.segments.r", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "Readable Bin Segments") },

    // Bin Sections
    { "bin.section", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "Current Bin Section") },
    { "bin.sections", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "All Bin Sections") },
    { "bin.sections.rwx", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "RWX Bin Sections") },
    { "bin.sections.r", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "Readable Bin Sections") },
    { "bin.sections.rw", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "RW Bin Sections") },
    { "bin.sections.rx", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "RX Bin Sections") },
    { "bin.sections.wx", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "WX Bin Sections") },
    { "bin.sections.x", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "Executable Bin Sections") },

    // IO Maps
    { "io.map", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "Current IO Map") },
    { "io.maps", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "All IO Maps") },
    { "io.maps.rwx", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "RWX IO Maps") },
    { "io.maps.r", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "Readable IO Maps") },
    { "io.maps.rw", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "RW IO Maps") },
    { "io.maps.rx", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "RX IO Maps") },
    { "io.maps.wx", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "WX IO Maps") },
    { "io.maps.x", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "Executable IO Maps") },

    // Debug
    { "dbg.stack", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "Debug Stack") },
    { "dbg.heap", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "Debug Heap") },
    { "dbg.map", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "Debug Map") },
    { "dbg.maps", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "Debug Maps") },
    { "dbg.maps.rwx", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "Debug Maps RWX") },
    { "dbg.maps.r", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "Debug Maps Readable") },
    { "dbg.maps.rw", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "Debug Maps RW") },
    { "dbg.maps.rx", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "Debug Maps RX") },
    { "dbg.maps.wx", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "Debug Maps WX") },
    { "dbg.maps.x", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "Debug Maps Executable") },

    // Analysis
    { "analysis.fcn", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "Function") },
    { "analysis.bb", QT_TRANSLATE_NOOP("AnalysisOptionsWidget", "Basic Block") }
};
}
AnalysisOptionsWidget::AnalysisOptionsWidget(PreferencesDialog *dialog)
    : QDialog(dialog), ui(new Ui::AnalysisOptionsWidget), mainWindow(dialog->getMainWindow())
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
    createAnalysisInOptionsList();

    // Connect each checkbox from "checkboxes" to the generic signal "checkboxEnabler"
    for (const ConfigCheckbox &confCheckbox : std::as_const(checkboxes)) {
        const QString val = confCheckbox.config;
        QCheckBox &cb = *confCheckbox.checkBox;
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
        connect(confCheckbox.checkBox, &QCheckBox::checkStateChanged, this,
                [val, &cb, this]() { checkboxEnabler(&cb, val); });
#else
        connect(confCheckbox.checkBox, &QCheckBox::stateChanged, this,
                [val, &cb, this]() { checkboxEnabler(&cb, val); });
#endif
    }

    ui->analyzePushButton->setToolTip(tr("Analyze the program using Rizin's \"aaa\" command"));
    connect(ui->analyzePushButton, &QPushButton::clicked, mainWindow,
            &MainWindow::onActionAnalyzeTriggered);
    connect<void (QComboBox::*)(int)>(ui->analysisInComboBox, &QComboBox::currentIndexChanged, this,
                                      &AnalysisOptionsWidget::updateAnalysisIn);
    connect<void (QSpinBox::*)(int)>(ui->ptrDepthSpinBox, &QSpinBox::valueChanged, this,
                                     &AnalysisOptionsWidget::updateAnalysisPtrDepth);
    connect(ui->preludeLineEdit, &QLineEdit::textChanged, this,
            &AnalysisOptionsWidget::updateAnalysisPrelude);

    connect(Core(), &CutterCore::analysisOptionsChanged, this,
            &AnalysisOptionsWidget::updateAnalysisOptionsFromVars);

    updateAnalysisOptionsFromVars();
}

AnalysisOptionsWidget::~AnalysisOptionsWidget() {}

void AnalysisOptionsWidget::checkboxEnabler(QCheckBox *checkBox, const QString &config)
{
    Config()->setConfig(config, checkBox->isChecked());
    this->analysisOptionsChanged();
}

void AnalysisOptionsWidget::updateAnalysisOptionsFromVars()
{
    for (const ConfigCheckbox &confCheckbox : std::as_const(checkboxes)) {
        qhelpers::setCheckedWithoutSignals(confCheckbox.checkBox,
                                           Core()->getConfigb(confCheckbox.config));
    }

    // Update the rest of analysis options that are not checkboxes
    ui->analysisInComboBox->setCurrentIndex(
            ui->analysisInComboBox->findData(Core()->getConfig("analysis.in")));
    ui->ptrDepthSpinBox->setValue(Core()->getConfigi("analysis.ptrdepth"));
    ui->preludeLineEdit->setText(Core()->getConfig("analysis.prelude"));
}

void AnalysisOptionsWidget::updateAnalysisIn(int index)
{
    Config()->setConfig("analysis.in", ui->analysisInComboBox->itemData(index).toString());
    this->analysisOptionsChanged();
}

void AnalysisOptionsWidget::updateAnalysisPtrDepth(int value)
{
    Config()->setConfig("analysis.ptrdepth", value);
    this->analysisOptionsChanged();
}

void AnalysisOptionsWidget::updateAnalysisPrelude(const QString &prelude)
{
    Config()->setConfig("analysis.prelude", prelude);
    this->analysisOptionsChanged();
}

void AnalysisOptionsWidget::createAnalysisInOptionsList()
{
    auto mapIter = analysisBoundaries.cbegin();
    ui->analysisInComboBox->blockSignals(true);
    ui->analysisInComboBox->clear();
    for (; mapIter != analysisBoundaries.cend(); ++mapIter) {
        ui->analysisInComboBox->addItem(tr(mapIter.value()), mapIter.key());
    }
    ui->analysisInComboBox->blockSignals(false);
}

void AnalysisOptionsWidget::analysisOptionsChanged() const
{
    disconnect(Core(), &CutterCore::analysisOptionsChanged, this,
               &AnalysisOptionsWidget::updateAnalysisOptionsFromVars);
    Core()->triggerAnalysisOptionsChanged();
    connect(Core(), &CutterCore::analysisOptionsChanged, this,
            &AnalysisOptionsWidget::updateAnalysisOptionsFromVars);
}
