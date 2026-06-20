#ifndef CUTTERDIFF_H
#define CUTTERDIFF_H

#include "Cutter.h"
#include "CutterCommon.h"
#include "RizinCpp.h"

#include <QMutex>
#include <QObject>

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
    enum : ut8 { AnalysisLevelSymbols, AnalysisLevelAuto, AnalysisLevelExperimental };

    enum : ut8 {
        CompareLogicDefault = 0, ///< Only symbols and functions (no imports)
        CompareLogicComplete, ///< All functions (imports included)
        CompareLogicSymbols, ///< Only symbols
    };
    QList<FunctionDescription> getFunctionList(bool orig = true);
    QByteArray ioRead(RVA addr, int len, bool orig);
    RzList *getFunctions(RzAnalysis *analysis, int compareLogic);
    QString getCommentAt(RVA addr, bool orig);
    QString listFlagsAsStringAt(RVA addr, bool orig);
    bool cmpBytesAt(RVA addrA, RVA addrB, size_t len);

private:
    RzCore *coreA = nullptr;
    RzCore *coreB = nullptr;
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    QMutex mutex;
#else
    QRecursiveMutex mutex;
#endif
signals:
};

#endif // CUTTERDIFF_H
