#include "Cutter.h"
#include "RunScriptTask.h"
#include "MainWindow.h"

RunScriptTask::RunScriptTask() :
    AsyncTask()
{
}

RunScriptTask::~RunScriptTask()
{
}

void RunScriptTask::interrupt()
{
    AsyncTask::interrupt();
    r_cons_singleton()->context->breaked = true;
}

void RunScriptTask::runTask()
{
    if (!this->fileName.isNull()) {
        log(tr("Executing script..."));
        Core()->cmdTask(". " + this->fileName);
        if (isInterrupted()) {
            return;
        }
    }
}
