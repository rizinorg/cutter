
#include "CommandTask.h"
#include "TempConfig.h"

CommandTask::CommandTask(const QString &cmd, ColorMode colorMode, bool outFormatHtml)
    : cmd(cmd), colorMode(colorMode), outFormatHtml(outFormatHtml)
{
}

void CommandTask::runTask() {
    TempConfig tempConfig;
    tempConfig.set("scr.color", colorMode);
    tempConfig.set("scr.html", outFormatHtml);
    auto res = Core()->cmdTask(cmd);
    emit finished(res);
}
