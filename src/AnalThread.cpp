#include "Cutter.h"
#include "AnalThread.h"
#include "MainWindow.h"
#include "dialogs/OptionsDialog.h"
#include <QJsonArray>
#include <QDebug>
#include <QCheckBox>

AnalThread::AnalThread(OptionsDialog *parent) :
    QThread(parent),
    level(2),
    main(nullptr),
    interrupted(false)
{
}

AnalThread::~AnalThread()
{
    if (isRunning()) {
        quit();
        wait();
    }
}

void AnalThread::start(MainWindow *main, int level, QList<QString> advanced)
{
    this->level = level;
    this->advanced = advanced;
    this->main = main;

    QThread::start();
}

void AnalThread::interruptAndWait()
{
    interrupted = true;

    while (isRunning()) {
        r_cons_singleton()->breaked = true;
        r_sys_usleep(10000);
    }
}

// run() will be called when a thread starts
void AnalThread::run()
{
    const auto optionsDialog = dynamic_cast<OptionsDialog *>(parent());
    const auto &ui = optionsDialog->ui;
    bool va = ui->vaCheckBox->isChecked();
    ut64 binLoadAddr = Core()->math(ui->entry_loadOffset->text()); // Where the bin header is located in the file (-B)
    ut64 mapAddr = Core()->math(ui->entry_mapOffset->text());      // Where to map the file once loaded (-m)
    interrupted = false;
    emit updateProgress(tr("Loading binary..."));

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
        Core()->loadFile(main->getFilename(), binLoadAddr, mapAddr, perms, va, loadBinInfo,
                       forceBinPlugin);
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
    emit updateProgress(tr("Analysis in progress..."));
    Core()->analyze(this->level, this->advanced);
    emit updateProgress(tr("Analysis complete!"));
}
