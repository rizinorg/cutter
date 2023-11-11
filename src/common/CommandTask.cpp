
#include "CommandTask.h"
#include "TempConfig.h"

CommandTask::CommandTask(const QString &cmd, ColorMode colorMode) : cmd(cmd), colorMode(colorMode)
{
}

void CommandTask::runTask()
{
    TempConfig tempConfig;
    tempConfig.set("scr.color", colorMode);
    auto res = Core()->cmdTask(cmd);
    emit finished(res);
}
