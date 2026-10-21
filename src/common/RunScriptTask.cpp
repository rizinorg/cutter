#include "common/RunScriptTask.h"

#include "core/Cutter.h"
#include "core/MainWindow.h"

RunScriptTask::RunScriptTask() : AsyncTask() {}

RunScriptTask::~RunScriptTask() {}

void RunScriptTask::interrupt()
{
    AsyncTask::interrupt();
    // assume caller holds the lock already so it should be safe to use raw rzCore here
    rz_interrupt_set_breaked(Core()->rzCore->intr, true);
}

void RunScriptTask::runTask()
{
    if (!this->fileName.isNull()) {
        log(tr("Executing script..."));
        Core()->functionTask([&](RzCore *core) {
            rz_core_run_script(core, this->fileName.toUtf8().constData());
            return nullptr;
        });
        if (isInterrupted()) {
            return;
        }
    }
}
