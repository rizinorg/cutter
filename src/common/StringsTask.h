#ifndef STRINGSTASK_H
#define STRINGSTASK_H

#include "common/AsyncTask.h"
#include "core/Cutter.h"

/**
 * @brief AsyncTask for querying all strings in binary
 */
class StringsTask : public AsyncTask
{
    Q_OBJECT

public:
    explicit StringsTask(bool raw) : raw(raw) {}
    QString getTitle() const override { return tr("Searching for Strings"); }

signals:
    void stringSearchFinished(const QList<StringDescription> &strings);

protected:
    void runTask() override
    {
        auto strings = Core()->getAllStrings(raw);
        emit stringSearchFinished(strings);
    }

private:
    bool raw;
};

#endif // STRINGSTASK_H
