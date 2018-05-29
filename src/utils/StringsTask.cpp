
#include "StringsTask.h"

void StringsTask::runTask()
{
    auto strings = Core()->getAllStrings();

    /*RCoreTask *task = Core()->startTask("izzj");
    Core()->joinTask(task);

    QList<StringDescription> strings = Core()->parseStringsJson(task->msg->res);*/

    emit stringSearchFinished(strings);
}
