
#include "CommandTask.h"
#include "TempConfig.h"

CommandTask::CommandTask(const QString &cmd, ColorMode colorMode, bool outFormatHtml)
    : cmd(cmd), colorMode(colorMode), outFormatHtml(outFormatHtml)
{
}

void CommandTask::runTask() {
    TempConfig tempConfig;
    tempConfig.set("scr.color", colorMode);
    auto res = Core()->cmdTask(cmd);
    if (outFormatHtml) {
        res = CutterCore::ansiEscapeToHtml(res);
    }
    emit finished(res);
}
