#ifndef CUTTER_BASEFIND_CORE_H
#define CUTTER_BASEFIND_CORE_H

#include <QThread>
#include <QMutex>

#include "Cutter.h"
#include "CutterDescriptions.h"
#include <rz_basefind.h>

class CutterCore;

class Basefind : public QThread
{
    Q_OBJECT

public:
    explicit Basefind(CutterCore *core);
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
    CutterCore *const core;
    RzList *scores;
    bool continue_run;
    RzBaseFindOpt options;
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    QMutex mutex;
#else
    QRecursiveMutex mutex;
#endif

    bool updateProgress(const RzBaseFindThreadInfo *info);
    static bool threadCallback(const RzBaseFindThreadInfo *info, void *user);
};

#endif // CUTTER_BASEFIND_CORE_H
