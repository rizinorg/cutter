
#include "CommandTask.h"

CommandTask::CommandTask(const QString &cmd)
    : cmd(cmd)
{
}

void CommandTask::runTask()
{
    auto res = Core()->cmdTask(cmd);
    emit finished(res);
}
