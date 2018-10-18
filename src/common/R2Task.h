
#ifndef R2TASK_H
#define R2TASK_H

#include "Cutter.h"

class R2Task: public QObject
{
    Q_OBJECT

private:
    RCoreTask *task;

    static void taskFinishedCallback(void *user, char *);
    void taskFinished();

public:
    explicit R2Task(const QString &cmd);
    ~R2Task();

    void startTask();
    void breakTask();
    void joinTask();

    QString getResult();
    const char *getResultRaw();

signals:
    void finished();
};

#endif // R2TASK_H