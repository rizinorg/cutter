#include "core/Cutter.h"
#include "common/RunScriptTask.h"
#include "core/MainWindow.h"

RunScriptTask::RunScriptTask() : AsyncTask() {}

RunScriptTask::~RunScriptTask() {}

void RunScriptTask::interrupt()
{
    AsyncTask::interrupt();
    rz_cons_singleton()->context->breaked = true;
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
