
#include "AsyncTask.h"

AsyncTask::AsyncTask(QObject *parent)
    : QObject(parent),
      QRunnable()
{
    setAutoDelete(false);
    running = false;
}

AsyncTask::~AsyncTask()
{
    wait();
}

void AsyncTask::wait()
{
    runningMutex.lock();
    runningMutex.unlock();
}

bool AsyncTask::wait(int timeout)
{
    bool r = runningMutex.tryLock(timeout);
    if (r) {
        runningMutex.unlock();
    }
    return r;
}

void AsyncTask::interrupt()
{
    interrupted = true;
}

void AsyncTask::prepareRun()
{
    interrupted = false;
    wait();
}

void AsyncTask::run()
{
    runningMutex.lock();
    running = true;
    logBuffer = "";
    emit logChanged(logBuffer);
    runTask();
    emit finished();
    running = false;
    runningMutex.unlock();
}

void AsyncTask::log(QString s)
{
    logBuffer += s;
    emit logChanged(logBuffer);
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
    task->prepareRun();
    threadPool->start(task);
}
