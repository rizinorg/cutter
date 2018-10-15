
#include "CommandTask.h"
#include "TempConfig.h"

CommandTask::CommandTask(const QString &cmd)
    : cmd(cmd)
{
}

void CommandTask::runTask()
{
    TempConfig tempConfig;
    int oldValue = tempConfig.get("scr.color");
    int newValue = COLOR_MODE_256;

    tempConfig.set("scr.color", newValue);
    tempConfig.set("scr.html", true);
    auto res = Core()->cmdTask(cmd);
    tempConfig.set("scr.color", oldValue);

    emit finished(res);
}
