#ifndef RUNSCRIPTTHREAD_H
#define RUNSCRIPTTHREAD_H

#include "common/AsyncTask.h"
#include "core/Cutter.h"

class RunScriptTask : public AsyncTask
{
    Q_OBJECT

public:
    explicit RunScriptTask();
    ~RunScriptTask();

    QString getTitle() override {
        return tr("Run Script");
    }

    void setFileName(const QString &fileName) {
        this->fileName = fileName;
    }

    void interrupt() override;

protected:
    void runTask() override;

private:
    QString fileName;
};

#endif // RUNSCRIPTTHREAD_H
