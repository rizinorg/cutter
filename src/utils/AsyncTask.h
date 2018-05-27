
#ifndef ASYNCTASK_H
#define ASYNCTASK_H

#include <QRunnable>
#include <QThreadPool>
#include <QMutex>
#include <QElapsedTimer>

class AsyncTaskManager;

class AsyncTask : public QObject, public QRunnable
{
    Q_OBJECT

    friend class AsyncTaskManager;

public:
    AsyncTask(QObject *parent = nullptr);
    ~AsyncTask();

    void run() override final;

    void wait();
    bool wait(int timeout);
    virtual void interrupt();
    bool isInterrupted()                { return interrupted; }
    bool isRunning()                    { return running; }

    const QString &getLog()             { return logBuffer; }
    const QElapsedTimer &getTimer()     { return timer; }

    virtual QString getTitle()          { return QString(); }

protected:
    virtual void runTask() =0;

    void log(QString s);

signals:
    void finished();
    void logChanged(const QString &log);

private:
    bool running;
    bool interrupted;
    QMutex runningMutex;

    QElapsedTimer timer;
    QString logBuffer;

    void prepareRun();
};


class AsyncTaskManager : public QObject
{
    Q_OBJECT

private:
    QThreadPool *threadPool;

public:
    explicit AsyncTaskManager(QObject *parent = nullptr);
    ~AsyncTaskManager();

    void start(AsyncTask *task);
};


#endif //ASYNCTASK_H