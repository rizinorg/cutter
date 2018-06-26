#include "Cutter.h"
#include "AnalTask.h"
#include "MainWindow.h"
#include "dialogs/OptionsDialog.h"
#include <QJsonArray>
#include <QDebug>
#include <QCheckBox>

AnalTask::AnalTask() :
    AsyncTask()
{
}

AnalTask::~AnalTask()
{
}

void AnalTask::interrupt()
{
    AsyncTask::interrupt();
    r_cons_singleton()->context->breaked = true;
}

void AnalTask::runTask()
{
    log(tr("Loading Binary...\n"));

    Core()->setCPU(options.arch, options.cpu, options.bits);

    int perms = R_IO_READ | R_IO_EXEC;
    if (options.writeEnabled)
        perms |= R_IO_WRITE;

    // Demangle (must be before file Core()->loadFile)
    Core()->setConfig("bin.demangle", options.demangle);

    // Do not reload the file if already loaded
    QJsonArray openedFiles = Core()->getOpenedFiles();
    if (!openedFiles.size()) {
        bool fileLoaded = Core()->loadFile(options.filename,
                                           options.binLoadAddr,
                                           options.mapAddr,
                                           perms,
                                           options.useVA,
                                           options.loadBinInfo,
                                           options.forceBinPlugin);
        if (!fileLoaded) {
            // Something wrong happened, fallback to open dialog
            emit openFileFailed();
            AsyncTask::interrupt();
            return;
        }
    }

    if (isInterrupted()) {
        return;
    }

    if (!options.os.isNull()) {
        Core()->cmd("e asm.os=" + options.os);
    }

    // Load PDB and/or scripts
    if (!options.pdbFile.isNull()) {
        Core()->loadPDB(options.pdbFile);
    }

    if (isInterrupted()) {
        return;
    }

    if (!options.script.isNull()) {
        Core()->loadScript(options.script);
    }

    if (isInterrupted()) {
        return;
    }

    if (options.endian != InitialOptions::Endianness::Auto) {
        Core()->setEndianness(options.endian == InitialOptions::Endianness::Big);
    }

    Core()->setBBSize(options.bbsize);

    // Use prj.simple as default as long as regular projects are broken
    Core()->setConfig("prj.simple", true);

    if (!options.analCmd.empty()) {
        log(tr("Analyzing...\n"));
        for (QString cmd : options.analCmd) {
            if (isInterrupted()) {
                return;
            }
            log("  " + tr("Running") + " " + cmd + "\n");
            Core()->cmd(cmd);
        }
        log(tr("Analysis complete!\n"));
    } else {
        log(tr("Skipping Analysis.\n"));
    }
}
