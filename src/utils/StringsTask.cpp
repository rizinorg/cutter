
#include "StringsTask.h"

void StringsTask::runTask()
{
    auto strings = Core()->getAllStrings();
    emit stringSearchFinished(strings);
}
