
#include "R2Task.h"

R2Task::R2Task(const QString &cmd)
{
    task = r_core_task_new(Core()->core(),
        true,
        cmd.toLocal8Bit().constData(),
        static_cast<RCoreTaskCallback>(&R2Task::taskFinishedCallback),
        this);
}

R2Task::~R2Task()
{
    r_core_task_del(Core()->core(), task->id);
}

void R2Task::taskFinishedCallback(void *user, char *)
{
    reinterpret_cast<R2Task *>(user)->taskFinished();
}

void R2Task::taskFinished()
{
    emit finished();
}

void R2Task::startTask()
{
    r_core_task_enqueue(Core()->core(), task);
}

void R2Task::breakTask()
{
    r_core_task_break(Core()->core(), task->id);
}

void R2Task::joinTask()
{
    r_core_task_join(Core()->core(), nullptr, task);
}

QString R2Task::getResult()
{
    return QString::fromUtf8(task->res);
}

const char *R2Task::getResultRaw()
{
    return task->res;
}
