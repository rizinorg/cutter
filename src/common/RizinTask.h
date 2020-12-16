
#ifndef R2TASK_H
#define R2TASK_H

#include "core/Cutter.h"

class RizinTask: public QObject
{
    Q_OBJECT

private:
    RzCoreTask *task;

    static void taskFinishedCallback(const char *, void *user);
    void taskFinished();

public:
    using Ptr = QSharedPointer<RizinTask>;

    explicit RizinTask(const QString &cmd, bool transient = true);
    ~RizinTask();

    void startTask();
    void breakTask();
    void joinTask();

    QString getResult();
    QJsonDocument getResultJson();
    const char *getResultRaw();

signals:
    void finished();
};

#endif // R2TASK_H
