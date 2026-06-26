#include "common/AnalysisTask.h"

#include "core/Cutter.h"
#include "core/MainWindow.h"
#include "dialogs/InitialOptionsDialog.h"

#include <QCheckBox>
#include <QDebug>
#include <QJsonArray>

AnalysisTask::AnalysisTask() : AsyncTask() {}

AnalysisTask::~AnalysisTask() {}

void AnalysisTask::interrupt()
{
    AsyncTask::interrupt();
    rz_cons_singleton()->context->breaked = true;
}

QString AnalysisTask::getTitle() const
{
    if (Core()->isFileLoaded()) {
        return tr("Initial Analysis");
    }
    return tr("Analyzing Program");
}

void AnalysisTask::runTask()
{
    int perms = RZ_PERM_RX;
    if (options.writeEnabled) {
        perms |= RZ_PERM_W;
        emit Core()->ioModeChanged();
    }
    // Need to lock Core here, so avoid the use of Core()-> functions as much as possible
    RzCoreLocked core(Core());

    // recursive mutex Demangle (must be before file Core()->loadFile)
    rz_config_set_b(core->config, "bin.demangle", options.demangle);

    // Do not reload the file if already loaded
    const RzList *descs = rz_id_storage_list(core->io->files);
    if (rz_list_empty(descs) && options.filename.length()) {
        log(tr("Loading the file..."));
        openFailed = false;
        const bool fileLoaded =
                Core()->loadFile(options.filename, options.binLoadAddr, options.mapAddr, perms,
                                 options.useVA, options.loadBinInfo, options.forceBinPlugin);
        if (!fileLoaded) {
            // Something wrong happened, fallback to open dialog
            openFailed = true;
            emit openFileFailed();
            interrupt();
            return;
        }
    }

    // rz_core_bin_load might change asm.bits, so let's set that after the bin is loaded
    Core()->setCPU(options.arch, options.cpu, options.bits);

    if (isInterrupted()) {
        return;
    }

    if (!options.os.isNull()) {
        rz_config_set(core->config, "asm.os", options.os.toUtf8().constData());
    }

    if (!options.pdbFile.isNull()) {
        log(tr("Loading PDB file..."));
        rz_core_bin_pdb_load(core, options.pdbFile.toUtf8().constData());
    }

    if (Core()->getConfigb("bin.dbginfo.debuginfod")) {
        log(tr("Applying Dwarf..."));
        rz_core_bin_apply_dwarf(core, rz_bin_cur(core->bin));
    }

    if (isInterrupted()) {
        return;
    }

    if (!options.shellcode.isNull() && options.shellcode.size() / 2 > 0) {
        log(tr("Loading shellcode..."));
        rz_core_write_hexpair(core, core->offset, options.shellcode.toStdString().c_str());
    }

    if (options.endian != InitialOptions::Endianness::Auto) {
        rz_config_set_b(core->config, "cfg.bigendian",
                        options.endian == InitialOptions::Endianness::Big);
    }

    rz_flag_space_set(core->flags, "*");

    if (!options.script.isNull()) {
        log(tr("Executing script..."));
        Core()->loadScript(options.script);
    }

    if (isInterrupted()) {
        return;
    }

    if (!options.analysisCmd.empty()) {
        log(tr("Executing analysis..."));
        for (const CommandDescription &cmd : std::as_const(options.analysisCmd)) {
            if (isInterrupted()) {
                return;
            }
            log(cmd.translatedDescription());
            // use cmd instead of cmdRaw because commands can be unexpected
            Core()->cmd(cmd.command);
        }
        log(tr("Analysis complete!"));
    } else {
        log(tr("Skipping Analysis."));
    }
}
