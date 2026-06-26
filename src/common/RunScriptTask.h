#ifndef RUNSCRIPTTASK_H
#define RUNSCRIPTTASK_H

#include "common/AsyncTask.h"

/**
 * @brief Async task for running rizin scripts
 */
class RunScriptTask : public AsyncTask
{
    Q_OBJECT

public:
    explicit RunScriptTask();
    ~RunScriptTask();

    QString getTitle() const override { return tr("Run Script"); }

    void setFileName(const QString &fileName) { this->fileName = fileName; }

    void interrupt() override;

protected:
    void runTask() override;

private:
    QString fileName;
};

#endif // RUNSCRIPTTASK_H
