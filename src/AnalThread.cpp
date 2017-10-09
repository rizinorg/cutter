#include <QDebug>
#include "cutter.h"
#include "AnalThread.h"
#include "MainWindow.h"
#include "Settings.h"
#include "dialogs/OptionsDialog.h"

AnalThread::AnalThread(OptionsDialog *parent) :
    QThread(parent),
    level(2),
    main(nullptr),
    core(CutterCore::getInstance())
{
}

AnalThread::~AnalThread()
{
    if (isRunning())
    {
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

// run() will be called when a thread starts
void AnalThread::run()
{
    const auto optionsDialog = dynamic_cast<OptionsDialog *>(parent());
    const auto& ui = optionsDialog->ui;
    int va = ui->vaCheckBox->isChecked();
    ut64 loadaddr = 0LL;
    ut64 mapaddr = 0LL;

    //
    // Advanced Options
    //

    core->setCPU(optionsDialog->getSelectedArch(), optionsDialog->getSelectedCPU(), optionsDialog->getSelectedBits());

    bool rw = false;
    bool load_bininfo = ui->binCheckBox->isChecked();

    if (load_bininfo)
    {
        if (!va)
        {
            va = 2;
            loadaddr = UT64_MAX;
            r_config_set_i(core->core()->config, "bin.laddr", loadaddr);
            mapaddr = 0;
        }
    }
    else
    {
        va = false;
        loadaddr = mapaddr = 0;
    }

    emit updateProgress(tr("Loading binary"));
    // options dialog should show the list of archs inside the given fatbin
    int binidx = 0; // index of subbin

    QString forceBinPlugin = nullptr;
    QVariant forceBinPluginData = ui->formatComboBox->currentData();
    if (!forceBinPluginData.isNull())
    {
        RBinPluginDescription pluginDesc = forceBinPluginData.value<RBinPluginDescription>();
        forceBinPlugin = pluginDesc.name;
    }

    core->setConfig("bin.demangle", ui->demangleCheckBox->isChecked());

    core->loadFile(main->getFilename(), loadaddr, mapaddr, rw, va, binidx, load_bininfo, forceBinPlugin);
    emit updateProgress("Analysis in progress.");

    QString os = optionsDialog->getSelectedOS();
    if (!os.isNull())
    {
        core->cmd("e asm.os=" + os);
    }


    if (ui->pdbCheckBox->isChecked())
    {
        core->loadPDB(ui->pdbLineEdit->text());
    }
    //qDebug() << "Anal level: " << this->level;
    core->analyze(this->level, this->advanced);
}
