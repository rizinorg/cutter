
#include "AsyncTask.h"

void AsyncTask::run()
{
    runTask();
    emit finished();
}

AsyncTaskManager::AsyncTaskManager(QObject *parent)
    : QObject(parent)
{
    threadPool = new QThreadPool(this);
}

AsyncTaskManager::~AsyncTaskManager()
{
}

void AsyncTaskManager::start(AsyncTask *task)
{
    threadPool->start(task);
}
