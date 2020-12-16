
#ifndef RZTASK_H
#define RZTASK_H

#include "core/Cutter.h"

class RizinTask: public QObject
{
    Q_OBJECT

protected:
    RzCoreTask *task;

    RizinTask() {}

public:
    using Ptr = QSharedPointer<RizinTask>;

    virtual ~RizinTask();

    void startTask();
    void breakTask();
    void joinTask();

signals:
    void finished();
};

class RizinCmdTask: public RizinTask
{
private:
    void taskFinished();
    static void taskFinishedCallback(const char *, void *user);

public:
    explicit RizinCmdTask(const QString &cmd, bool transient = true);

    QString getResult();
    QJsonDocument getResultJson();
    const char *getResultRaw();
};

#endif // RZTASK_H
