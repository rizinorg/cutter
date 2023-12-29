#ifndef CUTTER_BINDIFF_CORE_H
#define CUTTER_BINDIFF_CORE_H

#include <QThread>
#include <QMutex>

#include "Cutter.h"
#include "CutterDescriptions.h"
#include <rz_analysis.h>

class CutterCore;

class BinDiff : public QThread
{
    Q_OBJECT

public:
    explicit BinDiff(CutterCore *core);
    virtual ~BinDiff();

    void run(QString fileName);
    QList<BinDiffMatchDescription> matches();
    QList<FunctionDescription> mismatch(bool fileA);

public slots:
    void cancel();

signals:
    void progress(const size_t total, const size_t nLeft, const size_t nMatch);
    void complete();

private:
    CutterCore *const core;
    RzAnalysisMatchResult *result;
    bool continue_run;
    size_t maxTotal;
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    QMutex mutex;
#else
    QRecursiveMutex mutex;
#endif

    bool updateProgress(const size_t nLeft, const size_t nMatch);
    static bool threadCallback(const size_t nLeft, const size_t nMatch, void *user);
};

#endif // CUTTER_BINDIFF_CORE_H
