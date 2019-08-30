
#include "R2Task.h"

R2Task::R2Task(const QString &cmd, bool transient)
{
    task = r_core_task_new(Core()->core_,
        true,
        cmd.toLocal8Bit().constData(),
        static_cast<RCoreTaskCallback>(&R2Task::taskFinishedCallback),
        this);
    task->transient = transient;
    r_core_task_incref(task);
}

R2Task::~R2Task()
{
    r_core_task_decref(task);
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
    r_core_task_enqueue(Core()->core_, task);
}

void R2Task::breakTask()
{
    r_core_task_break(Core()->core_, task->id);
}

void R2Task::joinTask()
{
    r_core_task_join(Core()->core_, nullptr, task->id);
}

QString R2Task::getResult()
{
    return QString::fromUtf8(task->res);
}

QJsonDocument R2Task::getResultJson()
{
    return Core()->parseJson(task->res, task->cmd);
}

const char *R2Task::getResultRaw()
{
    return task->res;
}
