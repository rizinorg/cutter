
#include "AsyncTask.h"

#include <QDeadlineTimer>

#include <climits>
#include <utility>

AsyncTask::AsyncTask() : QObject(nullptr), QRunnable(), running(false)
{
    setAutoDelete(false);
}

AsyncTask::~AsyncTask()
{
    wait();
}

void AsyncTask::wait()
{
    QMutexLocker locker(&stateMutex);
    while (active) {
        doneCondition.wait(&stateMutex);
    }
}

bool AsyncTask::wait(int timeout)
{
    QMutexLocker locker(&stateMutex);
    const QDeadlineTimer deadline(timeout);
    while (active) {
        // wait() taking QDeadlineTimer as argument requires Qt >= 5.12, so the "ms" approach is
        // used here instead
        const qint64 remaining = deadline.remainingTime();
        const unsigned long waitMs =
                remaining < 0 ? ULONG_MAX : static_cast<unsigned long>(remaining);
        if (!doneCondition.wait(&stateMutex, waitMs)) {
            return !active;
        }
    }
    return true;
}

void AsyncTask::interrupt()
{
    interrupted = true;
}

void AsyncTask::prepareRun()
{
    interrupted = false;
    wait();
    {
        const QMutexLocker locker(&stateMutex);
        active = true;
    }
    timer.start();
}

void AsyncTask::run()
{
    {
        const QMutexLocker locker(&stateMutex);
        active = true;
    }

    running = true;

    logBuffer.clear();
    emit logChanged(logBuffer);
    runTask();

    running = false;

    emit finished();

    const QMutexLocker locker(&stateMutex);
    active = false;
    doneCondition.wakeAll();
}

void AsyncTask::log(QString s)
{
    logBuffer += s.append(QLatin1Char('\n'));
    emit logChanged(logBuffer);
}

AsyncTaskManager::AsyncTaskManager(QObject *parent)
    : QObject(parent), threadPool(new QThreadPool(this))
{
}

AsyncTaskManager::~AsyncTaskManager() {}

void AsyncTaskManager::start(const AsyncTask::Ptr &task)
{
    tasks.append(task);
    task->prepareRun();

    const std::weak_ptr<AsyncTask> weakPtr = task;
    connect(task.get(), &AsyncTask::finished, this, [this, weakPtr]() {
        tasks.removeOne(weakPtr.lock());
        emit tasksChanged();
    });
    threadPool->start(task.get());
    emit tasksChanged();
}

/**
 * @brief Interrupts every running task and waits until all have returned.
 */
void AsyncTaskManager::interruptAndWait()
{
    for (const AsyncTask::Ptr &task : std::as_const(tasks)) {
        task->interrupt();
    }
    threadPool->waitForDone();
}

bool AsyncTaskManager::getTasksRunning()
{
    return !tasks.isEmpty();
}
