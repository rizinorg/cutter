
#ifndef ASYNCTASK_H
#define ASYNCTASK_H

#include <QRunnable>
#include <QThreadPool>

class AsyncTask : public QObject, public QRunnable
{
    Q_OBJECT

public:
    void run() override final;

    virtual void runTask() =0;

signals:
    void finished();
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