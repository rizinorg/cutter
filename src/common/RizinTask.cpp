
#include "RizinTask.h"
#include <rz_core.h>

RizinTask::RizinTask(const QString &cmd, bool transient)
{
    task = rz_core_cmd_task_new(Core()->core(),
        cmd.toLocal8Bit().constData(),
        static_cast<RzCoreCmdTaskFinished>(&RizinTask::taskFinishedCallback),
        this);
    task->transient = transient;
    rz_core_task_incref(task);
}

RizinTask::~RizinTask()
{
    rz_core_task_decref(task);
}

void RizinTask::taskFinishedCallback(const char *, void *user)
{
    reinterpret_cast<RizinTask *>(user)->taskFinished();
}

void RizinTask::taskFinished()
{
    emit finished();
}

void RizinTask::startTask()
{
    rz_core_task_enqueue(&Core()->core_->tasks, task);
}

void RizinTask::breakTask()
{
    rz_core_task_break(&Core()->core_->tasks, task->id);
}

void RizinTask::joinTask()
{
    rz_core_task_join(&Core()->core_->tasks, nullptr, task->id);
}

QString RizinTask::getResult()
{
    const char *res = rz_core_cmd_task_get_result(task);
    if(!res) {
        return nullptr;
    }
    return QString::fromUtf8(res);
}

QJsonDocument RizinTask::getResultJson()
{
    const char *res = rz_core_cmd_task_get_result(task);
    if(!res) {
        return QJsonDocument();
    }
    return Core()->parseJson(res, nullptr);
}

const char *RizinTask::getResultRaw()
{
    return rz_core_cmd_task_get_result(task);
}
