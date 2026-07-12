#ifndef CUTTER_BINDIFF_CORE_H
#define CUTTER_BINDIFF_CORE_H

#include "Cutter.h"
#include "CutterDescriptions.h"
#include "CutterDiff.h"

#include <QMutex>
#include <QThread>

#include <rz_analysis.h>

class CutterDiffWindow;
/**
 * @brief The BinDiff class
 * Thread run for processing the functional diffing and other large diffing processes.
 * TODO: Move to common or tools/bindiff?
 */
class BinDiff : public QThread
{
    Q_OBJECT
    friend class CutterDiffWindow;

public:
    explicit BinDiff();
    virtual ~BinDiff();

    void run();

    void setFileA(QString filePth);
    void setFileB(QString filePth);
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
    std::unique_ptr<CutterDiff> cutterDiff;
    RzAnalysisMatchResult *result;
    void sortFunctions();
    QList<BinDiffMatchDescription> matchedList;
    QSet<const RzAnalysisFunction *> removedSet;
    QSet<const RzAnalysisFunction *> addedSet;
    bool continueRun;
    size_t maxTotal;
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    QMutex mutex;
#else
    QRecursiveMutex mutex;
#endif
    QString fileA;
    QString fileB;
    int level;
    int compareLogic;

    bool updateProgress(const size_t nLeft, const size_t nMatch);
    static bool threadCallback(const size_t nLeft, const size_t nMatch, void *user);
};

#endif // CUTTER_BINDIFF_CORE_H
