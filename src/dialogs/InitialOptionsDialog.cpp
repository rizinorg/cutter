#include "InitialOptionsDialog.h"

#include "CutterApplication.h"
#include "common/AnalysisTask.h"
#include "common/AsyncTask.h"
#include "common/Helpers.h"
#include "core/Cutter.h"
#include "core/MainWindow.h"
#include "dialogs/AsyncTaskDialog.h"
#include "dialogs/NewFileDialog.h"
#include "ui_InitialOptionsDialog.h"

#include <QCloseEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QSettings>

QString CommandDescription::translatedDescription() const
{
    return QCoreApplication::translate("InitialOptionsDialog", description);
}

InitialOptionsDialog::InitialOptionsDialog(MainWindow *main)
    : QDialog(nullptr), // parent must not be main
      ui(new Ui::InitialOptionsDialog),
      main(main)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
    ui->logoSvgWidget->load(Config()->getLogoFile());
    ui->debuginfodCheckBox->setChecked(Core()->getConfigb("bin.dbginfo.debuginfod"));
    ui->debuginfodLineEdit->setText(Core()->getConfig("bin.dbginfo.debuginfod_urls"));

    // Fill the plugins combo
    asmPlugins = Core()->getRAsmPluginDescriptions();
    for (const auto &plugin : std::as_const(asmPlugins)) {
        ui->archComboBox->addItem(plugin.name, plugin.name);
    }

    setTooltipWithConfigHelp(ui->archComboBox, "asm.arch");

    // cpu combo box
    ui->cpuComboBox->lineEdit()->setPlaceholderText(tr("Auto"));
    setTooltipWithConfigHelp(ui->cpuComboBox, "asm.cpu");

    updateCPUComboBox();

    // os combo box
    for (const auto &plugin : Core()->getConfigOptions("asm.os")) {
        ui->kernelComboBox->addItem(plugin, plugin);
    }

    setTooltipWithConfigHelp(ui->kernelComboBox, "asm.os");
    setTooltipWithConfigHelp(ui->bitsComboBox, "asm.bits");

    for (const auto &plugin : Core()->getBinPluginDescriptions(true, false)) {
        ui->formatComboBox->addItem(plugin.name, QVariant::fromValue(plugin));
    }

    analysisCommands = {
        { { "aa", QT_TRANSLATE_NOOP("InitialOptionsDialog", "Analyze all symbols") },
          new QCheckBox(),
          true },
        { { "aar",
            QT_TRANSLATE_NOOP("InitialOptionsDialog", "Analyze instructions for references") },
          new QCheckBox(),
          true },
        { { "aac", QT_TRANSLATE_NOOP("InitialOptionsDialog", "Analyze function calls") },
          new QCheckBox(),
          true },
        { { "aab", QT_TRANSLATE_NOOP("InitialOptionsDialog", "Analyze all basic blocks") },
          new QCheckBox(),
          false },
        { { "aao", QT_TRANSLATE_NOOP("InitialOptionsDialog", "Analyze all objc references") },
          new QCheckBox(),
          false },
        { { "avrr",
            QT_TRANSLATE_NOOP("InitialOptionsDialog", "Recover class information from RTTI") },
          new QCheckBox(),
          false },
        { { "aan",
            QT_TRANSLATE_NOOP("InitialOptionsDialog", "Autoname functions based on context") },
          new QCheckBox(),
          false },
        { { "aae",
            QT_TRANSLATE_NOOP("InitialOptionsDialog", "Emulate code to find computed references") },
          new QCheckBox(),
          false },
        { { "aafr",
            QT_TRANSLATE_NOOP("InitialOptionsDialog", "Analyze all consecutive functions") },
          new QCheckBox(),
          false },
        { { "aaft",
            QT_TRANSLATE_NOOP("InitialOptionsDialog", "Type and Argument matching analysis") },
          new QCheckBox(),
          false },
        { { "aaT", QT_TRANSLATE_NOOP("InitialOptionsDialog", "Analyze code after trap-sleds") },
          new QCheckBox(),
          false },
        { { "aap", QT_TRANSLATE_NOOP("InitialOptionsDialog", "Analyze function preludes") },
          new QCheckBox(),
          false },
        { { "e! analysis.jmp.tbl",
            QT_TRANSLATE_NOOP("InitialOptionsDialog", "Analyze jump tables in switch statements") },
          new QCheckBox(),
          false },
        { { "e! analysis.pushret",
            QT_TRANSLATE_NOOP("InitialOptionsDialog", "Analyze PUSH+RET as JMP") },
          new QCheckBox(),
          false },
        { { "e! analysis.hasnext",
            QT_TRANSLATE_NOOP("InitialOptionsDialog", "Continue analysis after each function") },
          new QCheckBox(),
          false }
    };

    // Per each checkbox, set a tooltip desccribing it
    AnalysisCommands item;
    foreach (item, analysisCommands) {
        item.checkbox->setText(item.commandDesc.translatedDescription());
        item.checkbox->setToolTip(item.commandDesc.command);
        item.checkbox->setChecked(item.checked);
        ui->verticalLayout7->addWidget(item.checkbox);
    }

    ui->hideFrame->setVisible(false);
    ui->analysisoptionsFrame->setVisible(false);
    ui->advancedAnlysisLine->setVisible(false);

    updateDebuginfodLayout();
    updatePDBLayout();

#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    connect(ui->pdbCheckBox, &QCheckBox::checkStateChanged, this,
            &InitialOptionsDialog::updatePDBLayout);
    connect(ui->scriptCheckBox, &QCheckBox::checkStateChanged, this,
            &InitialOptionsDialog::updateScriptLayout);
    connect(ui->debuginfodCheckBox, &QCheckBox::checkStateChanged, this,
            &InitialOptionsDialog::updateDebuginfodLayout);
#else
    connect(ui->pdbCheckBox, &QCheckBox::stateChanged, this,
            &InitialOptionsDialog::updatePDBLayout);
    connect(ui->scriptCheckBox, &QCheckBox::stateChanged, this,
            &InitialOptionsDialog::updateScriptLayout);
    connect(ui->debuginfodCheckBox, &QCheckBox::stateChanged, this,
            &InitialOptionsDialog::updateDebuginfodLayout);
#endif
    updateScriptLayout();

    connect(ui->cancelButton, &QPushButton::clicked, this, &InitialOptionsDialog::reject);

    ui->programLineEdit->setText(main->getFilename());

    connect(ui->okButton, &QPushButton::clicked, this, &InitialOptionsDialog::onOkButtonClicked);
    connect(ui->analysisSlider, &QSlider::valueChanged, this,
            &InitialOptionsDialog::onAnalysisSliderValueChanged);
    connect(ui->analysisCheckBox, &QCheckBox::clicked, this,
            &InitialOptionsDialog::onAnalysisCheckBoxClicked);
    connect(ui->advOptButton, &QToolButton::clicked, this,
            &InitialOptionsDialog::onAdvOptButtonClicked);
    connect(ui->archComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &InitialOptionsDialog::onArchComboBoxCurrentIndexChanged);
    connect(ui->pdbSelectButton, &QPushButton::clicked, this,
            &InitialOptionsDialog::onPdbSelectButtonClicked);
    connect(ui->scriptSelectButton, &QPushButton::clicked, this,
            &InitialOptionsDialog::onScriptSelectButtonClicked);
}

void InitialOptionsDialog::updateDebuginfodLayout()
{
    ui->debuginfodWidget->setEnabled(ui->debuginfodCheckBox->isChecked());
}

InitialOptionsDialog::~InitialOptionsDialog() {}

void InitialOptionsDialog::updateCPUComboBox()
{
    const QString currentText = ui->cpuComboBox->lineEdit()->text();
    ui->cpuComboBox->clear();

    QString arch = getSelectedArch();
    QStringList cpus;
    if (!arch.isEmpty()) {
        auto pluginDescr = std::find_if(
                asmPlugins.begin(), asmPlugins.end(),
                [&](const RzAsmPluginDescription &plugin) { return plugin.name == arch; });
        if (pluginDescr != asmPlugins.end()) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
            cpus = pluginDescr->cpus.split(",", Qt::SkipEmptyParts);
#else
            cpus = pluginDescr->cpus.split(",", QString::SkipEmptyParts);
#endif
        }
    }

    ui->cpuComboBox->addItem("");
    ui->cpuComboBox->addItems(cpus);

    ui->cpuComboBox->lineEdit()->setText(currentText);
}

QList<QString> InitialOptionsDialog::getAnalysisCommands(const InitialOptions &options)
{
    QList<QString> commands;
    for (auto &commandDesc : options.analysisCmd) {
        commands << commandDesc.command;
    }
    return commands;
}

void InitialOptionsDialog::loadOptions(const InitialOptions &options)
{
    if (options.analysisCmd.isEmpty()) {
        analysisLevel = 0;
    } else if (options.analysisCmd.first().command == "aaa") {
        analysisLevel = 1;
    } else if (options.analysisCmd.first().command == "aaaa") {
        analysisLevel = 2;
    } else {
        analysisLevel = 3;
        AnalysisCommands item;
        const QList<QString> commands = getAnalysisCommands(options);
        foreach (item, analysisCommands) {
            qInfo() << item.commandDesc.command;
            item.checkbox->setChecked(commands.contains(item.commandDesc.command));
        }
    }

    if (!options.script.isEmpty()) {
        ui->scriptCheckBox->setChecked(true);
        ui->scriptLineEdit->setText(options.script);
        analysisLevel = 0;
    } else {
        ui->scriptCheckBox->setChecked(false);
        ui->scriptLineEdit->setText("");
    }

    ui->analysisSlider->setValue(analysisLevel);

    shellcode = options.shellcode;

    if (!options.forceBinPlugin.isEmpty()) {
        ui->formatComboBox->setCurrentText(options.forceBinPlugin);
    } else {
        ui->formatComboBox->setCurrentIndex(0);
    }

    if (options.binLoadAddr != RVA_INVALID) {
        ui->entryLoadOffset->setText(rzAddressString(options.binLoadAddr));
    }

    if (options.mapAddr != RVA_INVALID) {
        ui->entryMapOffset->setText(rzAddressString(options.mapAddr));
    }

    ui->vaCheckBox->setChecked(options.useVA);
    ui->writeCheckBox->setChecked(options.writeEnabled);

    if (!options.arch.isNull() && !options.arch.isEmpty()) {
        ui->archComboBox->setCurrentText(options.arch);
    }

    if (!options.cpu.isNull() && !options.cpu.isEmpty()) {
        ui->cpuComboBox->setCurrentText(options.cpu);
    }

    if (options.bits > 0) {
        ui->bitsComboBox->setCurrentText(QString::asprintf("%d", options.bits));
    }

    if (!options.os.isNull() && !options.os.isEmpty()) {
        ui->kernelComboBox->setCurrentText(options.os);
    }

    if (!options.forceBinPlugin.isNull() && !options.forceBinPlugin.isEmpty()) {
        ui->formatComboBox->setCurrentText(options.forceBinPlugin);
    }

    if (!options.loadBinInfo) {
        ui->binCheckBox->setChecked(false);
    }

    ui->writeCheckBox->setChecked(options.writeEnabled);

    switch (options.endian) {
    case InitialOptions::Endianness::Little:
        ui->endiannessComboBox->setCurrentIndex(1);
        break;
    case InitialOptions::Endianness::Big:
        ui->endiannessComboBox->setCurrentIndex(2);
        break;
    default:
        break;
    }

    ui->demangleCheckBox->setChecked(options.demangle);

    if (!options.pdbFile.isNull() && !options.pdbFile.isEmpty()) {
        ui->pdbCheckBox->setChecked(true);
        ui->pdbLineEdit->setText(options.pdbFile);
    }
}

void InitialOptionsDialog::setTooltipWithConfigHelp(QWidget *w, const char *config)
{
    w->setToolTip(QString("%1 (%2)").arg(Core()->getConfigDescription(config)).arg(config));
}

QString InitialOptionsDialog::getSelectedArch() const
{
    const QVariant archValue = ui->archComboBox->currentData();
    return archValue.isValid() ? archValue.toString() : nullptr;
}

QString InitialOptionsDialog::getSelectedCPU() const
{
    QString cpu = ui->cpuComboBox->currentText();
    if (cpu.isNull() || cpu.isEmpty()) {
        return nullptr;
    }
    return cpu;
}

int InitialOptionsDialog::getSelectedBits() const
{
    const QString selBits = ui->bitsComboBox->currentText();
    if (selBits != "Auto") { // TODO: #3187 using potentially trtanslated text for logic
        return selBits.toInt();
    }

    return 0;
}

InitialOptions::Endianness InitialOptionsDialog::getSelectedEndianness() const
{
    switch (ui->endiannessComboBox->currentIndex()) {
    case 1:
        return InitialOptions::Endianness::Little;
    case 2:
        return InitialOptions::Endianness::Big;
    default:
        return InitialOptions::Endianness::Auto;
    }
}

QString InitialOptionsDialog::getSelectedOS() const
{
    const QVariant os = ui->kernelComboBox->currentData();
    return os.isValid() ? os.toString() : nullptr;
}

QList<CommandDescription> InitialOptionsDialog::getSelectedAdvancedAnalCmds() const
{
    QList<CommandDescription> advanced = QList<CommandDescription>();
    if (ui->analysisSlider->value() == 3) {
        AnalysisCommands item;
        foreach (item, analysisCommands) {
            if (item.checkbox->isChecked()) {
                advanced << item.commandDesc;
            }
        }
    }
    return advanced;
}

void InitialOptionsDialog::setupAndStartAnalysis()
{
    InitialOptions options;

    options.filename = main->getFilename();
    if (!options.filename.isEmpty()) {
        main->setWindowTitle("Cutter – " + options.filename);
    }
    options.shellcode = this->shellcode;

    // Where the bin header is located in the file (-B)
    if (ui->entryLoadOffset->text().length() > 0) {
        options.binLoadAddr = Core()->math(ui->entryLoadOffset->text());
    }

    options.mapAddr =
            Core()->math(ui->entryMapOffset->text()); // Where to map the file once loaded (-m)
    options.arch = getSelectedArch();
    options.cpu = getSelectedCPU();
    options.bits = getSelectedBits();
    options.os = getSelectedOS();
    options.writeEnabled = ui->writeCheckBox->isChecked();
    options.loadBinInfo = !ui->binCheckBox->isChecked();
    options.useVA = ui->vaCheckBox->isChecked();
    const QVariant forceBinPluginData = ui->formatComboBox->currentData();
    if (!forceBinPluginData.isNull()) {
        const auto pluginDesc = forceBinPluginData.value<RzBinPluginDescription>();
        options.forceBinPlugin = pluginDesc.name;
    }
    options.demangle = ui->demangleCheckBox->isChecked();
    if (ui->pdbCheckBox->isChecked()) {
        options.pdbFile = ui->pdbLineEdit->text();
    }
    if (ui->scriptCheckBox->isChecked()) {
        options.script = ui->scriptLineEdit->text();
    }
    if (ui->debuginfodCheckBox->isChecked()) {
        Core()->setConfig("bin.dbginfo.debuginfod", true);
        Core()->setConfig("bin.dbginfo.debuginfod_urls", ui->debuginfodLineEdit->text());
    }

    options.endian = getSelectedEndianness();

    const int level = ui->analysisSlider->value();
    switch (level) {
    case 1:
        options.analysisCmd = { { "aaa",
                                  QT_TRANSLATE_NOOP("InitialOptionsDialog", "Auto analysis") } };
        break;
    case 2:
        options.analysisCmd = {
            { "aaaa", QT_TRANSLATE_NOOP("InitialOptionsDialog", "Auto analysis (experimental)") }
        };
        break;
    case 3:
        options.analysisCmd = getSelectedAdvancedAnalCmds();
        break;
    default:
        options.analysisCmd = {};
        break;
    }

    auto *analysisTask = new AnalysisTask();
    analysisTask->setOptions(options);

    MainWindow *main = this->main;
    connect(analysisTask, &AnalysisTask::openFileFailed, main, &MainWindow::openNewFileFailed);
    connect(analysisTask, &AsyncTask::finished, main, [analysisTask, main]() {
        if (analysisTask->getOpenFileFailed()) {
            return;
        }
        main->finalizeOpen();
    });

    const AsyncTask::Ptr analysisTaskPtr(analysisTask);

    auto *taskDialog = new AsyncTaskDialog(analysisTaskPtr);

    taskDialog->setInterruptOnClose(true);
    taskDialog->setAttribute(Qt::WA_DeleteOnClose);
    taskDialog->show();

    Core()->getAsyncTaskManager()->start(analysisTaskPtr);

    done(0);

    static_cast<CutterApplication *>(qApp)->setInitialOptions(options);
}

void InitialOptionsDialog::onOkButtonClicked()
{
    ui->okButton->setEnabled(false);
    setupAndStartAnalysis();
}

void InitialOptionsDialog::closeEvent(QCloseEvent *event)
{
    event->accept();
}

QString InitialOptionsDialog::analysisDescription(int level)
{
    // TODO: replace this with meaningful descriptions
    switch (level) {
    case 0:
        return tr("No analysis");
    case 1:
        return tr("Auto-Analysis (aaa)");
    case 2:
        return tr("Auto-Analysis Experimental (aaaa)");
    case 3:
        return tr("Advanced");
    default:
        return tr("Unknown");
    }
}

void InitialOptionsDialog::onAnalysisSliderValueChanged(int value)
{
    ui->analDescription->setText(tr("Level") + QString(": %1").arg(analysisDescription(value)));
    if (value == 0) {
        ui->analysisCheckBox->setChecked(false);
        ui->analysisCheckBox->setText(tr("Analysis: Disabled"));
    } else {
        ui->analysisCheckBox->setChecked(true);
        ui->analysisCheckBox->setText(tr("Analysis: Enabled"));
        if (value == 3) {
            ui->analysisoptionsFrame->setVisible(true);
            ui->advancedAnlysisLine->setVisible(true);
        } else {
            ui->analysisoptionsFrame->setVisible(false);
            ui->advancedAnlysisLine->setVisible(false);
        }
    }
}

void InitialOptionsDialog::onAdvOptButtonClicked()
{
    if (ui->advOptButton->isChecked()) {
        ui->hideFrame->setVisible(true);
        ui->advOptButton->setArrowType(Qt::DownArrow);
    } else {
        ui->hideFrame->setVisible(false);
        ui->advOptButton->setArrowType(Qt::RightArrow);
    }
}

void InitialOptionsDialog::onAnalysisCheckBoxClicked(bool checked)
{
    if (!checked) {
        analysisLevel = ui->analysisSlider->value();
    }
    ui->analysisSlider->setValue(checked ? analysisLevel : 0);
}

void InitialOptionsDialog::onArchComboBoxCurrentIndexChanged(int)
{
    updateCPUComboBox();
}

void InitialOptionsDialog::updatePDBLayout()
{
    ui->pdbWidget->setEnabled(ui->pdbCheckBox->isChecked());
}

void InitialOptionsDialog::onPdbSelectButtonClicked()
{
    QFileDialog dialog(this);
    dialog.setWindowTitle(tr("Select PDB file"));
    dialog.setNameFilters({ tr("PDB file (*.pdb)"), tr("All files (*)") });

    if (!dialog.exec()) {
        return;
    }

    const auto files = dialog.selectedFiles();
    if (files.first().isEmpty()) {
        return;
    }
    const QString &fileName = QDir::toNativeSeparators(files.first());

    if (!fileName.isEmpty()) {
        ui->pdbLineEdit->setText(fileName);
    }
}

void InitialOptionsDialog::updateScriptLayout()
{
    ui->scriptWidget->setEnabled(ui->scriptCheckBox->isChecked());
}

void InitialOptionsDialog::onScriptSelectButtonClicked()
{
    QFileDialog dialog(this);
    dialog.setWindowTitle(tr("Select Rizin script file"));
    dialog.setNameFilters({ tr("Script file (*.rz)"), tr("All files (*)") });

    if (!dialog.exec()) {
        return;
    }

    const auto files = dialog.selectedFiles();
    if (files.first().isEmpty()) {
        return;
    }
    const QString &fileName = QDir::toNativeSeparators(files.first());

    if (!fileName.isEmpty()) {
        ui->scriptLineEdit->setText(fileName);
    }
}

void InitialOptionsDialog::reject()
{
    done(0);
    main->displayNewFileDialog();
}
