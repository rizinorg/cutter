#include "Cutter.h"
#include "AnalTask.h"
#include "MainWindow.h"
#include "dialogs/OptionsDialog.h"
#include <QJsonArray>
#include <QDebug>
#include <QCheckBox>

AnalTask::AnalTask(OptionsDialog *parent) :
    AsyncTask(parent),
    level(2),
    main(nullptr)
{
}

AnalTask::~AnalTask()
{
}

void AnalTask::setSettings(MainWindow *main, int level, QList<QString> advanced)
{
    this->main = main;
    this->level = level;
    this->advanced = advanced;
}

void AnalTask::interrupt()
{
    AsyncTask::interrupt();
    r_cons_singleton()->breaked = true;
}

void AnalTask::interruptAndWait()
{
    do {
        interrupt();
    } while(!wait(10));
}

void AnalTask::runTask()
{
    const auto optionsDialog = dynamic_cast<OptionsDialog *>(parent());
    const auto &ui = optionsDialog->ui;
    bool va = ui->vaCheckBox->isChecked();
    ut64 binLoadAddr = UT64_MAX;                                   // Where the bin header is located in the file (-B)
    if (ui->entry_loadOffset->text().length() > 0)
        binLoadAddr = Core()->math(ui->entry_loadOffset->text());
    ut64 mapAddr = Core()->math(ui->entry_mapOffset->text());      // Where to map the file once loaded (-m)

    log(tr("Loading Binary...\n"));

    // Set the CPU details (handle auto)
    Core()->setCPU(optionsDialog->getSelectedArch(), optionsDialog->getSelectedCPU(),
                 optionsDialog->getSelectedBits());

    // Binary opening permissions (read/write/execute)
    int perms = R_IO_READ | R_IO_EXEC;
    if (ui->writeCheckBox->isChecked())
        perms |= R_IO_WRITE;

    // Check if we must load and parse binary header (ELF, PE, ...)
    bool loadBinInfo = !ui->binCheckBox->isChecked();
    QString forceBinPlugin = nullptr;
    QVariant forceBinPluginData = ui->formatComboBox->currentData();
    if (!forceBinPluginData.isNull()) {
        RBinPluginDescription pluginDesc = forceBinPluginData.value<RBinPluginDescription>();
        forceBinPlugin = pluginDesc.name;
    }

    // Demangle (must be before file Core()->loadFile)
    Core()->setConfig("bin.demangle", ui->demangleCheckBox->isChecked());

    // Do not reload the file if already loaded
    QJsonArray openedFiles = Core()->getOpenedFiles();
    if (!openedFiles.size()) {
        bool fileLoaded = Core()->loadFile(main->getFilename(), binLoadAddr, mapAddr, perms, va, loadBinInfo,
                       forceBinPlugin);
        if (!fileLoaded) {
            // Something wrong happened, fallback to open dialog
            emit openFileFailed();
            AsyncTask::interrupt();
            return;
        }
    }

    // Set asm OS configuration
    QString os = optionsDialog->getSelectedOS();
    if (!os.isNull()) {
        Core()->cmd("e asm.os=" + os);
    }

    // Load PDB and/or scripts
    if (ui->pdbCheckBox->isChecked()) {
        Core()->loadPDB(ui->pdbLineEdit->text());
    }
    if (ui->scriptCheckBox->isChecked()) {
        Core()->loadScript(ui->scriptLineEdit->text());
    }

    // Set various options
    if (optionsDialog->getSelectedEndianness() != OptionsDialog::Endianness::Auto) {
        Core()->setEndianness(optionsDialog->getSelectedEndianness() == OptionsDialog::Endianness::Big);
    }
    Core()->setBBSize(optionsDialog->getSelectedBBSize());
    // Use prj.simple as default as long as regular projects are broken
    Core()->setConfig("prj.simple", true);

    // Start analysis
    log(tr("Analysis in progress...\n"));

    Core()->analyze(this->level, this->advanced);
    
    log(tr("Analysis complete!"));
}
