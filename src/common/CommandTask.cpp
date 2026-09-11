#include "CommandTask.h"

#include "Cutter.h"
#include "TempConfig.h"

CommandTask::CommandTask(const QString &cmd, ColorMode colorMode) : cmd(cmd), colorMode(colorMode)
{
}

void CommandTask::runTask()
{
    TempConfig tempConfig;
    tempConfig.set("scr.color", colorMode);
    auto res = Session()->cmdTask(cmd);
    emit finished(res);
}
