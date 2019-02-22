
#ifndef FUNCTIONSTASK_H
#define FUNCTIONSTASK_H

#include "common/AsyncTask.h"
#include "core/Cutter.h"

class FunctionsTask : public AsyncTask
{
Q_OBJECT

public:
    QString getTitle() override                     { return tr("Fetching Functions"); }

signals:
    void fetchFinished(const QList<FunctionDescription> &strings);

protected:
    void runTask() override
    {
        auto functions = Core()->getAllFunctions();
        emit fetchFinished(functions);
    }
};

#endif //FUNCTIONSTASK_H
