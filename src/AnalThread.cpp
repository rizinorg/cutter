#include <QDebug>
#include "Cutter.h"
#include "AnalThread.h"
#include "MainWindow.h"
#include "dialogs/OptionsDialog.h"
#include <QJsonArray>

AnalThread::AnalThread(OptionsDialog *parent) :
    QThread(parent),
    level(2),
    main(nullptr),
    core(Core()),
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
    int va = ui->vaCheckBox->isChecked();
    ut64 loadaddr = 0LL;
    ut64 mapaddr = 0LL;

    interrupted = false;

    //
    // Advanced Options
    //

    core->setCPU(optionsDialog->getSelectedArch(), optionsDialog->getSelectedCPU(),
                 optionsDialog->getSelectedBits());

    int perms = R_IO_READ | R_IO_EXEC;
    if (ui->writeCheckBox->isChecked())
        perms |= R_IO_WRITE;
    bool loadBinInfo = !ui->binCheckBox->isChecked();

    if (loadBinInfo) {
        if (!va) {
            va = 2;
            loadaddr = UT64_MAX;
            r_config_set_i(core->core()->config, "bin.laddr", loadaddr);
            mapaddr = 0;
        }
    } else {
        Core()->setConfig("file.info", "false");
        va = false;
        loadaddr = mapaddr = 0;
    }

    emit updateProgress(tr("Loading binary"));
    // options dialog should show the list of archs inside the given fatbin
    int binidx = 0; // index of subbin

    QString forceBinPlugin = nullptr;
    QVariant forceBinPluginData = ui->formatComboBox->currentData();
    if (!forceBinPluginData.isNull()) {
        RBinPluginDescription pluginDesc = forceBinPluginData.value<RBinPluginDescription>();
        forceBinPlugin = pluginDesc.name;
    }

    core->setConfig("bin.demangle", ui->demangleCheckBox->isChecked());

    QJsonArray openedFiles = Core()->getOpenedFiles();
    if (!openedFiles.size()) {
        core->loadFile(main->getFilename(), loadaddr, mapaddr, perms, va, binidx, loadBinInfo,
                       forceBinPlugin);
    }
    emit updateProgress("Analysis in progress.");

    QString os = optionsDialog->getSelectedOS();
    if (!os.isNull()) {
        core->cmd("e asm.os=" + os);
    }

    if (ui->pdbCheckBox->isChecked()) {
        core->loadPDB(ui->pdbLineEdit->text());
    }

    if (optionsDialog->getSelectedEndianness() != OptionsDialog::Endianness::Auto) {
        core->setEndianness(optionsDialog->getSelectedEndianness() == OptionsDialog::Endianness::Big);
    }

    core->setBBSize(optionsDialog->getSelectedBBSize());

    // use prj.simple as default as long as regular projects are broken
    core->setConfig("prj.simple", true);

    core->analyze(this->level, this->advanced);
}
