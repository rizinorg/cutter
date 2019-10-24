
#ifndef STRINGSASYNCTASK_H
#define STRINGSASYNCTASK_H

#include "common/AsyncTask.h"
#include "core/Cutter.h"

class StringsTask : public AsyncTask
{
Q_OBJECT

public:
    QString getTitle() override                     { return tr("Searching for Strings"); }

signals:
    void stringSearchFinished(const QList<StringDescription> &strings);

protected:
    void runTask() override
    {
        auto strings = Core()->getAllStrings();
        emit stringSearchFinished(strings);
    }
};

#endif //STRINGSASYNCTASK_H
