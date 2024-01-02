#ifndef CUTTER_BINDIFF_CORE_H
#define CUTTER_BINDIFF_CORE_H

#include <QThread>
#include <QMutex>

#include "Cutter.h"
#include "CutterDescriptions.h"
#include <rz_analysis.h>

class BinDiff : public QThread
{
    Q_OBJECT

public:
    explicit BinDiff();
    virtual ~BinDiff();

    void run();

    void setFile(QString filePth);
    void setAnalysisLevel(int aLevel);
    void setCompareLogic(int cLogic);
    bool hasData();

    QList<BinDiffMatchDescription> matches();
    QList<FunctionDescription> mismatch(bool originalFile);

public slots:
    void cancel();

signals:
    void progress(BinDiffStatusDescription status);
    void complete();

private:
    RzAnalysisMatchResult *result;
    bool continueRun;
    size_t maxTotal;
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    QMutex mutex;
#else
    QRecursiveMutex mutex;
#endif
    QString file;
    int level;
    int compareLogic;

    bool updateProgress(const size_t nLeft, const size_t nMatch);
    static bool threadCallback(const size_t nLeft, const size_t nMatch, void *user);
};

#endif // CUTTER_BINDIFF_CORE_H
