#include "core/Cutter.h"
#include "common/AnalTask.h"
#include "core/MainWindow.h"
#include "dialogs/InitialOptionsDialog.h"
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
    log(tr("Loading the file..."));
    openFailed = false;

    int perms = R_PERM_RX;
    if (options.writeEnabled)
        perms |= R_PERM_W;

    // Demangle (must be before file Core()->loadFile)
    Core()->setConfig("bin.demangle", options.demangle);

    // Do not reload the file if already loaded
    QJsonArray openedFiles = Core()->getOpenedFiles();
    if (!openedFiles.size() && options.filename.length()) {
        bool fileLoaded = Core()->loadFile(options.filename,
                                           options.binLoadAddr,
                                           options.mapAddr,
                                           perms,
                                           options.useVA,
                                           options.loadBinInfo,
                                           options.forceBinPlugin);
        if (!fileLoaded) {
            // Something wrong happened, fallback to open dialog
            openFailed = true;
            emit openFileFailed();
            interrupt();
            return;
        }
    }

    // r_core_bin_load might change asm.bits, so let's set that after the bin is loaded
    Core()->setCPU(options.arch, options.cpu, options.bits);

    if (isInterrupted()) {
        return;
    }

    if (!options.os.isNull()) {
        Core()->cmd("e asm.os=" + options.os);
    }

    if (!options.pdbFile.isNull()) {
        log(tr("Loading PDB file..."));
        Core()->loadPDB(options.pdbFile);
    }

    if (isInterrupted()) {
        return;
    }

    if (!options.shellcode.isNull() && options.shellcode.size() / 2 > 0) {
        log(tr("Loading shellcode..."));
        Core()->cmd("wx " + options.shellcode);
    }

    if (options.endian != InitialOptions::Endianness::Auto) {
        Core()->setEndianness(options.endian == InitialOptions::Endianness::Big);
    }

    Core()->cmd("fs *");

    if (!options.script.isNull()) {
        log(tr("Executing script..."));
        Core()->loadScript(options.script);
    }

    if (isInterrupted()) {
        return;
    }

    // Use prj.simple as default as long as regular projects are broken
    Core()->setConfig("prj.simple", true);

    if (!options.analCmd.empty()) {
        log(tr("Analyzing..."));
        for (const QString &cmd : options.analCmd) {
            if (isInterrupted()) {
                return;
            }
            log("  " + tr("Running") + " " + cmd);
            Core()->cmd(cmd);
        }
        log(tr("Analysis complete!"));
    } else {
        log(tr("Skipping Analysis."));
    }
}
