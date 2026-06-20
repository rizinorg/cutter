#ifndef CUTTERDIFF_H
#define CUTTERDIFF_H

#include <QObject>
#include <QMutex>


#include "RizinCpp.h"
#include "CutterCommon.h"
#include "Cutter.h"


class RzDiffCoreLocked;

class CUTTER_EXPORT CutterDiff : public QObject
{
    Q_OBJECT
public:
    explicit CutterDiff(QObject *parent = nullptr);
    ~CutterDiff();
    bool initCores();
    bool openFiles(const QString &fileA, const QString &fileB);
    bool analyzeCores(int level);
    void syncConfig();
    RzAnalysisMatchResult *matchFunctions(int compareLogic, RzAnalysisMatchThreadInfoCb callback,
                                          void *user);
    enum : ut8 {
        AnalysisLevelSymbols,
        AnalysisLevelAuto,
        AnalysisLevelExperimental
    };

    enum : ut8 {
        CompareLogicDefault = 0, ///< Only symbols and functions (no imports)
        CompareLogicComplete, ///< All functions (imports included)
        CompareLogicSymbols, ///< Only symbols
    };
    QList<FunctionDescription> getFunctionList(bool orig = true);

private:
    RzCore *coreA = nullptr;
    RzCore *coreB = nullptr;
    void storeFunctions();
    QRecursiveMutex mutex;
    RzList *getFunctions(RzAnalysis *analysis, int compareLogic);
signals:
};

#endif // CUTTERDIFF_H
