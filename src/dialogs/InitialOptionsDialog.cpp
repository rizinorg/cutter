#include "common/AsyncTask.h"
#include "InitialOptionsDialog.h"
#include "ui_InitialOptionsDialog.h"

#include "core/MainWindow.h"
#include "dialogs/NewFileDialog.h"
#include "dialogs/AsyncTaskDialog.h"
#include "common/Helpers.h"

#include <QSettings>
#include <QFileInfo>
#include <QFileDialog>
#include <QCloseEvent>

#include "core/Cutter.h"
#include "common/AnalTask.h"


InitialOptionsDialog::InitialOptionsDialog(MainWindow *main):
    QDialog(nullptr), // parent must not be main
    ui(new Ui::InitialOptionsDialog),
    main(main),
    core(Core())
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
    ui->logoSvgWidget->load(Config()->getLogoFile());

    // Fill the plugins combo
    asmPlugins = core->getRAsmPluginDescriptions();
    for (const auto &plugin : asmPlugins) {
        ui->archComboBox->addItem(plugin.name, plugin.name);
    }

    setTooltipWithConfigHelp(ui->archComboBox,"asm.arch");

    // cpu combo box
    ui->cpuComboBox->lineEdit()->setPlaceholderText(tr("Auto"));
    setTooltipWithConfigHelp(ui->cpuComboBox, "asm.cpu");

    updateCPUComboBox();

    // os combo box
    for (const auto &plugin : core->cmdList("e asm.os=?")) {
        ui->kernelComboBox->addItem(plugin, plugin);
    }

    setTooltipWithConfigHelp(ui->kernelComboBox, "asm.os");
    setTooltipWithConfigHelp(ui->bitsComboBox, "asm.bits");

    for (const auto &plugin : core->getRBinPluginDescriptions("bin")) {
        ui->formatComboBox->addItem(plugin.name, QVariant::fromValue(plugin));
    }

    analysisCommands = {
        { { "aa", tr("Analyze all symbols") }, new QCheckBox(), true },
        { { "aar", tr("Analyze instructions for references") }, new QCheckBox(), true },
        { { "aac", tr("Analyze function calls") }, new QCheckBox(), true },
        { { "aab", tr("Analyze all basic blocks") }, new QCheckBox(), false },
        { { "aao", tr("Analyze all objc references") }, new QCheckBox(), false },
        { { "avrr", tr("Recover class information from RTTI") }, new QCheckBox(), false },
        { { "aan", tr("Autoname functions based on context") }, new QCheckBox(), false },
        { { "aae", tr("Emulate code to find computed references") }, new QCheckBox(), false },
        { { "aafr", tr("Analyze all consecutive functions") }, new QCheckBox(), false },
        { { "aaft", tr("Type and Argument matching analysis") }, new QCheckBox(), false },
        { { "aaT", tr("Analyze code after trap-sleds") }, new QCheckBox(), false },
        { { "aap", tr("Analyze function preludes") }, new QCheckBox(), false },
        { { "e! anal.jmp.tbl", tr("Analyze jump tables in switch statements") }, new QCheckBox(), false },
        { { "e! anal.pushret", tr("Analyze PUSH+RET as JMP") },  new QCheckBox(), false },
        { { "e! anal.hasnext", tr("Continue analysis after each function") }, new QCheckBox(), false }};

    // Per each checkbox, set a tooltip desccribing it
    AnalysisCommands item;
    foreach (item, analysisCommands){
        item.checkbox->setText(item.commandDesc.description);
        item.checkbox->setToolTip(item.commandDesc.command);
        item.checkbox->setChecked(item.checked);
        ui->verticalLayout_7->addWidget(item.checkbox);
    }
    

    ui->hideFrame->setVisible(false);
    ui->analoptionsFrame->setVisible(false);
    ui->advancedAnlysisLine->setVisible(false);

    updatePDBLayout();

    connect(ui->pdbCheckBox, SIGNAL(stateChanged(int)), this, SLOT(updatePDBLayout()));

    updateScriptLayout();

    connect(ui->scriptCheckBox, SIGNAL(stateChanged(int)), this, SLOT(updateScriptLayout()));

    connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

    ui->programLineEdit->setText(main->getFilename());
}

InitialOptionsDialog::~InitialOptionsDialog() {}

void InitialOptionsDialog::updateCPUComboBox()
{
    QString currentText = ui->cpuComboBox->lineEdit()->text();
    ui->cpuComboBox->clear();

    QString arch = getSelectedArch();
    QStringList cpus;
    if (!arch.isEmpty()) {
        auto pluginDescr = std::find_if(asmPlugins.begin(), asmPlugins.end(), [&](const RAsmPluginDescription &plugin) {
            return plugin.name == arch;
        });
        if (pluginDescr != asmPlugins.end()) {
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
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

QList<QString> InitialOptionsDialog::getAnalysisCommands(const InitialOptions &options) {
    QList<QString> commands;
    for (auto& commandDesc: options.analCmd) {
        commands << commandDesc.command;
    }
    return commands;
}

void InitialOptionsDialog::loadOptions(const InitialOptions &options)
{
    if (options.analCmd.isEmpty()) {
        analLevel = 0;
    } else if (options.analCmd.first().command == "aaa" ) {
        analLevel = 1;
    } else if (options.analCmd.first().command ==  "aaaa" ) {
        analLevel = 2;
    } else {
        analLevel = 3;
        AnalysisCommands item;
        QList<QString> commands = getAnalysisCommands(options);
        foreach (item, analysisCommands){
            qInfo() << item.commandDesc.command;
            item.checkbox->setChecked(commands.contains(item.commandDesc.command));
        }
    }

    if (!options.script.isEmpty()) {
        ui->scriptCheckBox->setChecked(true);
        ui->scriptLineEdit->setText(options.script);
        analLevel = 0;
    } else {
        ui->scriptCheckBox->setChecked(false);
        ui->scriptLineEdit->setText("");
    }

    ui->analSlider->setValue(analLevel);

    shellcode = options.shellcode;

    if (!options.forceBinPlugin.isEmpty()) {
        ui->formatComboBox->setCurrentText(options.forceBinPlugin);
    } else {
        ui->formatComboBox->setCurrentIndex(0);
    }

    if (options.binLoadAddr != RVA_INVALID) {
        ui->entry_loadOffset->setText(RAddressString(options.binLoadAddr));
    }

	ui->writeCheckBox->setChecked(options.writeEnabled);


    // TODO: all other options should also be applied to the ui
}


void InitialOptionsDialog::setTooltipWithConfigHelp(QWidget *w, const char *config) {
    w->setToolTip(QString("%1 (%2)")
                  .arg(core->getConfigDescription(config))
                  .arg(config));
}


QString InitialOptionsDialog::getSelectedArch() const
{
    QVariant archValue = ui->archComboBox->currentData();
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
    QString sel_bits = ui->bitsComboBox->currentText();
    if (sel_bits != "Auto") {
        return sel_bits.toInt();
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
    QVariant os = ui->kernelComboBox->currentData();
    return os.isValid() ? os.toString() : nullptr;
}

QList<CommandDescription> InitialOptionsDialog::getSelectedAdvancedAnalCmds() const
{
    QList<CommandDescription> advanced = QList<CommandDescription>();
    if (ui->analSlider->value() == 3) {
        AnalysisCommands item;
        foreach (item, analysisCommands){
            if(item.checkbox->isChecked()) {
                advanced << item.commandDesc;
            }
        }
    }
    return advanced;
}

void InitialOptionsDialog::setupAndStartAnalysis(/*int level, QList<QString> advanced*/)
{
    InitialOptions options;

    options.filename = main->getFilename();
    if (!options.filename.isEmpty()) {
        main->setWindowTitle("Cutter – " + options.filename);
    }
    options.shellcode = this->shellcode;

    // Where the bin header is located in the file (-B)
    if (ui->entry_loadOffset->text().length() > 0) {
        options.binLoadAddr = Core()->math(ui->entry_loadOffset->text());
    }

    options.mapAddr = Core()->math(
                          ui->entry_mapOffset->text());      // Where to map the file once loaded (-m)
    options.arch = getSelectedArch();
    options.cpu = getSelectedCPU();
    options.bits = getSelectedBits();
    options.os = getSelectedOS();
    options.writeEnabled = ui->writeCheckBox->isChecked();
    options.loadBinInfo = !ui->binCheckBox->isChecked();
    QVariant forceBinPluginData = ui->formatComboBox->currentData();
    if (!forceBinPluginData.isNull()) {
        RBinPluginDescription pluginDesc = forceBinPluginData.value<RBinPluginDescription>();
        options.forceBinPlugin = pluginDesc.name;
    }
    options.demangle = ui->demangleCheckBox->isChecked();
    if (ui->pdbCheckBox->isChecked()) {
        options.pdbFile = ui->pdbLineEdit->text();
    }
    if (ui->scriptCheckBox->isChecked()) {
        options.script = ui->scriptLineEdit->text();
    }


    options.endian = getSelectedEndianness();

    int level = ui->analSlider->value();
    switch (level) {
    case 1:
        options.analCmd = { {"aaa", "Auto analysis"} };
        break;
    case 2:
        options.analCmd = { {"aaaa", "Auto analysis (experimental)"} };
        break;
    case 3:
        options.analCmd = getSelectedAdvancedAnalCmds();
        break;
    default:
        options.analCmd = {};
        break;
    }


    AnalTask *analTask = new AnalTask();
    analTask->setOptions(options);

    MainWindow *main = this->main;
    connect(analTask, &AnalTask::openFileFailed, main, &MainWindow::openNewFileFailed);
    connect(analTask, &AsyncTask::finished, main, [analTask, main]() {
        if (analTask->getOpenFileFailed()) {
            return;
        }
        main->finalizeOpen();
    });

    AsyncTask::Ptr analTaskPtr(analTask);

    AsyncTaskDialog *taskDialog = new AsyncTaskDialog(analTaskPtr);
    taskDialog->setInterruptOnClose(true);
    taskDialog->setAttribute(Qt::WA_DeleteOnClose);
    taskDialog->show();

    Core()->getAsyncTaskManager()->start(analTaskPtr);

    done(0);
}

void InitialOptionsDialog::on_okButton_clicked()
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
    //TODO: replace this with meaningful descriptions
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

void InitialOptionsDialog::on_analSlider_valueChanged(int value)
{
    ui->analDescription->setText(tr("Level") + QString(": %1").arg(analysisDescription(value)));
    if (value == 0) {
        ui->analCheckBox->setChecked(false);
        ui->analCheckBox->setText(tr("Analysis: Disabled"));
    } else {
        ui->analCheckBox->setChecked(true);
        ui->analCheckBox->setText(tr("Analysis: Enabled"));
        if (value == 3) {
            ui->analoptionsFrame->setVisible(true);
            ui->advancedAnlysisLine->setVisible(true);
        } else {
            ui->analoptionsFrame->setVisible(false);
            ui->advancedAnlysisLine->setVisible(false);
        }
    }
}

void InitialOptionsDialog::on_AdvOptButton_clicked()
{
    if (ui->AdvOptButton->isChecked()) {
        ui->hideFrame->setVisible(true);
        ui->AdvOptButton->setArrowType(Qt::DownArrow);
    } else {
        ui->hideFrame->setVisible(false);
        ui->AdvOptButton->setArrowType(Qt::RightArrow);
    }
}

void InitialOptionsDialog::on_analCheckBox_clicked(bool checked)
{
    if (!checked) {
        analLevel = ui->analSlider->value();
    }
    ui->analSlider->setValue(checked ? analLevel : 0);
}

void InitialOptionsDialog::on_archComboBox_currentIndexChanged(int)
{
    updateCPUComboBox();
}

void InitialOptionsDialog::updatePDBLayout()
{
    ui->pdbWidget->setEnabled(ui->pdbCheckBox->isChecked());
}

void InitialOptionsDialog::on_pdbSelectButton_clicked()
{
    QFileDialog dialog(this);
    dialog.setWindowTitle(tr("Select PDB file"));
    dialog.setNameFilters({ tr("PDB file (*.pdb)"), tr("All files (*)") });

    if (!dialog.exec()) {
        return;
    }

    const QString &fileName = QDir::toNativeSeparators(dialog.selectedFiles().first());

    if (!fileName.isEmpty()) {
        ui->pdbLineEdit->setText(fileName);
    }
}


void InitialOptionsDialog::updateScriptLayout()
{
    ui->scriptWidget->setEnabled(ui->scriptCheckBox->isChecked());
}

void InitialOptionsDialog::on_scriptSelectButton_clicked()
{
    QFileDialog dialog(this);
    dialog.setWindowTitle(tr("Select radare2 script file"));
    dialog.setNameFilters({ tr("Script file (*.r2)"), tr("All files (*)") });

    if (!dialog.exec()) {
        return;
    }

    const QString &fileName = QDir::toNativeSeparators(dialog.selectedFiles().first());

    if (!fileName.isEmpty()) {
        ui->scriptLineEdit->setText(fileName);
    }
}


void InitialOptionsDialog::reject()
{
    done(0);
    main->displayNewFileDialog();
}
