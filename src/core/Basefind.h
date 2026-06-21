#ifndef BASEFIND_H
#define BASEFIND_H

#include "CutterDescriptions.h"

#include <QMutex>
#include <QThread>

#include <rz_basefind.h>

class CutterCore;

/**
 * @brief Threaded wrapper for Rizin's basefind functionality to identify the base address of a
 * binary
 */
class Basefind : public QThread
{
    Q_OBJECT

public:
    explicit Basefind();
    virtual ~Basefind();

    void run();
    bool setOptions(const RzBaseFindOpt *opts);
    QList<BasefindResultDescription> results();

public slots:
    void cancel();

signals:
    void progress(BasefindCoreStatusDescription status);
    void complete();

private:
    RzList *scores;
    bool continueRun;
    RzBaseFindOpt options;
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    QMutex mutex;
#else
    QRecursiveMutex mutex;
#endif

    bool updateProgress(const RzBaseFindThreadInfo *info);
    static bool threadCallback(const RzBaseFindThreadInfo *info, void *user);
};

#endif // BASEFIND_H
