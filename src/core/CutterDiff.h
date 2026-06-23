#ifndef CUTTERDIFF_H
#define CUTTERDIFF_H

#include "Cutter.h"
#include "CutterCommon.h"
#include "RizinCpp.h"

#include <QMutex>
#include <QMutexLocker>
#include <QObject>

class CutterDiffLocked;

class CUTTER_EXPORT CutterDiff : public QObject
{
    Q_OBJECT
    friend class CutterDiffLocked;

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
    QByteArray ioRead(RVA addr, int len, bool orig = true);
    QString getCommentAt(RVA addr, bool orig = true);
    QString listFlagsAsStringAt(RVA addr, bool orig = true);
    bool cmpBytesAt(RVA addrA, RVA addrB, size_t len);
    QString getString(RVA addr, uint64_t len, RzStrEnc encoding, bool escape_nl = false,
                      bool orig = true);
    QString getCommonConfig(const char *key);
    QString getCommonConfig(const QString &key)
    {
        return getCommonConfig(key.toUtf8().constData());
    }
    void setCommonConfig(const char *key, const char *value);
    int getCommonConfigi(const char *key);
    int getCommonConfigi(const QString &key) { return getCommonConfigi(key.toUtf8().constData()); }
    bool getCommonConfigb(const char *key);
    bool getCommonConfigb(const QString &key) { return getCommonConfigb(key.toUtf8().constData()); }
    void setCommonConfigi(const char *key, int value);
    void setCommonConfigb(const char *key, bool value);
    void seekSilent(ut64 offset, bool orig);
    QString cmdRawAt(const char *cmd, RVA address, bool orig);
    QString cmdRaw(const char *cmd, bool orig);
    RVA getOffset(bool orig);

private:
    RzCore *coreA = nullptr;
    RzCore *coreB = nullptr;
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    QMutex mutex;
#else
    QRecursiveMutex mutex;
#endif
    RzList *getFunctions(RzAnalysis *analysis, int compareLogic);
signals:
};

class CutterDiffLocked
{
    CutterDiff *const diff;

public:
    RzCore *const coreA;
    RzCore *const coreB;
    explicit CutterDiffLocked(CutterDiff *diff) : diff(diff), coreA(diff->coreA), coreB(diff->coreB)
    {
        assert(diff);
        diff->mutex.lock();
    }
    ~CutterDiffLocked() { diff->mutex.unlock(); }
    CutterDiff *operator->() & { return diff; }
    CutterDiff *operator->() const & { return diff; }
    CutterDiff *operator->() && = delete;
};

#endif // CUTTERDIFF_H
