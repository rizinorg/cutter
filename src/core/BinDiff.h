#ifndef CUTTER_BINDIFF_CORE_H
#define CUTTER_BINDIFF_CORE_H

#include "Cutter.h"
#include "CutterDescriptions.h"
#include "CutterDiff.h"

#include <QMutex>
#include <QThread>

#include <rz_analysis.h>

struct BinDiffOptions
{
    QString fileA;
    QString fileB;

    int analysisLevel = 0;
    int compareLogic = 0;
};

class BinDiffJob : public QObject
{
    Q_OBJECT
public:
    BinDiffJob(CutterDiff *cutterDiff, const BinDiffOptions &options, QObject *parent = nullptr)
        : QObject(parent), cutterDiff(cutterDiff), options(options)
    {
    }

    virtual ~BinDiffJob() = default;

    virtual QString name() const = 0;
    virtual bool run() = 0;
    virtual double progress() const = 0;
    virtual void cancel() { canContinue = false; }

signals:
    void progressUpdated(double progress);

protected:
    void updateProgress(double progress)
    {
        currentProgress = progress;
        emit progressUpdated(progress);
    }

    CutterDiff *cutterDiff;
    const BinDiffOptions options;
    double currentProgress = 0.0;
    bool canContinue = true;
};

class CutterDiffInitJob : public BinDiffJob
{
    Q_OBJECT

public:
    CutterDiffInitJob(CutterDiff *cutterDiff, const BinDiffOptions &options,
                      QObject *parent = nullptr)
        : BinDiffJob(cutterDiff, options, parent)
    {
    }

    QString name() const override { return "Initialize"; }

    bool run() override
    {
        updateProgress(0.0);

        cutterDiff->initCores();
        cutterDiff->syncConfig();
        cutterDiff->openFiles(options.fileA, options.fileB);
        cutterDiff->analyzeCores(options.analysisLevel);

        updateProgress(1.0);

        return true;
    }

    double progress() const override { return currentProgress; }
};

class FunctionMatchJob : public BinDiffJob
{
    Q_OBJECT

public:
    FunctionMatchJob(CutterDiff *cutterDiff, const BinDiffOptions &options,
                     QObject *parent = nullptr);

    QString name() const override { return "Match Functions"; }

    bool run() override;

    double progress() const override { return currentProgress; }

private:
    struct MatchEntry
    {
        const RzAnalysisFunction *fcnA;
        const RzAnalysisFunction *fcnB;
        double similarity;
    };
    static bool threadCallback(const size_t nLeft, const size_t nMatch, void *user);
    int maxTotal = 1;
    bool validateProgress(const size_t nLeft, const size_t nMatches);
};

class BlocksMatchJob : public BinDiffJob
{
    Q_OBJECT

public:
    BlocksMatchJob(CutterDiff *cutterDiff, const BinDiffOptions &options,
                   QObject *parent = nullptr);

    QString name() const override { return "Match Blocks"; }

    bool run() override;

    double progress() const override { return currentProgress; }

private:
    int parsedFunctions = 0;
    struct MatchBlockEntry
    {
        const RzAnalysisBlock *blockA;
        const RzAnalysisBlock *blockB;
        double similarity;
    };
    static bool threadCallback(const size_t nLeft, const size_t nMatch, void *user);
    int maxTotal = 1;
    bool validateProgress(const size_t nLeft, const size_t nMatches);
};

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
    explicit BinDiff(CutterDiff *cutterDiff, const BinDiffOptions &options);
    virtual ~BinDiff();

    void run();
public slots:
    void cancel();

signals:
    void progress(BinDiffStatusDescription status);
    void complete();

private:
    const BinDiffOptions options;
    CutterDiff *cutterDiff;
    bool continueRun;
    int completedJobs = 0;
    BinDiffJob *currentJob = nullptr;
    QList<BinDiffJob *> diffJobs;
    size_t maxTotal;
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    QMutex mutex;
#else
    QRecursiveMutex mutex;
#endif
    bool updateProgress(double currentProgress);
};

#endif // CUTTER_BINDIFF_CORE_H
