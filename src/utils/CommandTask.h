
#ifndef COMMANDTASK_H
#define COMMANDTASK_H

#include "utils/AsyncTask.h"
#include "Cutter.h"

class CommandTask : public AsyncTask
{
Q_OBJECT

public:
    CommandTask(const QString &cmd);

    QString getTitle() override                     { return tr("Running Command"); }

signals:
    void finished(const QString &result);

protected:
    void runTask() override;

private:
    QString cmd;
};

#endif //COMMANDTASK_H
