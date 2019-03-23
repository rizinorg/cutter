
#include "AsyncTask.h"

AsyncTask::AsyncTask()
    : QObject(nullptr),
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
    timer.start();
}

void AsyncTask::run()
{
    runningMutex.lock();

    running = true;

    logBuffer.clear();
    emit logChanged(logBuffer);
    runTask();

    running = false;

    emit finished();

    runningMutex.unlock();
}

void AsyncTask::log(QString s)
{
    logBuffer += s.append(QLatin1Char('\n'));
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

void AsyncTaskManager::start(AsyncTask::Ptr task)
{
    tasks.append(task);
    task->prepareRun();

    QWeakPointer<AsyncTask> weakPtr = task;
    connect(task.data(), &AsyncTask::finished, this, [this, weakPtr]() {
        tasks.removeOne(weakPtr);
        emit tasksChanged();
    });
    threadPool->start(task.data());
    emit tasksChanged();
}

bool AsyncTaskManager::getTasksRunning()
{
    return !tasks.isEmpty();
}
