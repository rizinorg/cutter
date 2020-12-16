
#include "RizinTask.h"
#include <rz_core.h>

RizinTask::~RizinTask()
{
    if (task) {
        rz_core_task_decref(task);
    }
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

// RizinCmdTask

RizinCmdTask::RizinCmdTask(const QString &cmd, bool transient)
{
    task = rz_core_cmd_task_new(Core()->core(),
        cmd.toLocal8Bit().constData(),
        static_cast<RzCoreCmdTaskFinished>(&RizinCmdTask::taskFinishedCallback),
        this);
    task->transient = transient;
    rz_core_task_incref(task);
}

void RizinCmdTask::taskFinishedCallback(const char *, void *user)
{
    reinterpret_cast<RizinCmdTask *>(user)->taskFinished();
}

void RizinCmdTask::taskFinished()
{
    emit finished();
}

QString RizinCmdTask::getResult()
{
    const char *res = rz_core_cmd_task_get_result(task);
    if(!res) {
        return nullptr;
    }
    return QString::fromUtf8(res);
}

QJsonDocument RizinCmdTask::getResultJson()
{
    const char *res = rz_core_cmd_task_get_result(task);
    if(!res) {
        return QJsonDocument();
    }
    return Core()->parseJson(res, nullptr);
}

const char *RizinCmdTask::getResultRaw()
{
    return rz_core_cmd_task_get_result(task);
}
