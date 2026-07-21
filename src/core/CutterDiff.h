#ifndef CUTTERDIFF_H
#define CUTTERDIFF_H

#include "Cutter.h"
#include "CutterCommon.h"
#include "RizinCpp.h"

#include <QMutex>
#include <QMutexLocker>
#include <QObject>

class CutterDiffLocked;

struct Bound
{
    int pos;
    int size;
};

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
    RzDiff *lineDiff(const char *lines1, const char *lines2); // Own RzDiff
    RzDiff *lineDiff(const QString &lines1, const QString &lines2); // Own RzDiff
    RzDiff *diffFunctionDissas(RVA addrA, RVA addrB); // Own RzDiff
    QString diffFunctionDecomp(RVA addrA, RVA addrB);
    QString diffFunctionRzIL(RVA addrA, RVA addrB);
    QString disassembleFunction(RVA addr, bool orig);
    QString ansiEscapeToHtml(const QString &text);
    CutterRzList<RzList /*<RzDiffOp *>*/> lineDiffOpsGrouped(RzDiff *diff) const;
    QString getFileName(bool orig = true) const { return orig ? fileNameA : fileNameB; }
    QString getFilePath(bool orig = true) const { return orig ? filePathA : filePathB; }
    Bound getLineDiffBounds(const QString &line1, const QString &line2);

private:
    RzCore *coreA = nullptr;
    RzCore *coreB = nullptr;
    QString fileNameA;
    QString fileNameB;
    QString filePathA;
    QString filePathB;
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    QMutex mutex;
#else
    QRecursiveMutex mutex;
#endif
    RzList *getFunctions(RzAnalysis *analysis, int compareLogic); // Own RzList
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

class TempDiffConfig
{
public:
    explicit TempDiffConfig(CutterDiff *cutterDiff, bool orig = true)
        : cutterDiff(cutterDiff), orig(orig)
    {
    }

    ~TempDiffConfig()
    {
        const CutterDiffLocked lock(cutterDiff);
        const RzCore *const core = orig ? lock.coreA : lock.coreB;

        for (auto it = iHash.cbegin(); it != iHash.cend(); ++it) {
            const QByteArray key = it.key().toUtf8();
            rz_config_set_i(core->config, key.constData(), it.value());
        }

        for (auto it = bHash.cbegin(); it != bHash.cend(); ++it) {
            const QByteArray key = it.key().toUtf8();
            rz_config_set_b(core->config, key.constData(), it.value());
        }

        for (auto it = cHash.cbegin(); it != cHash.cend(); ++it) {
            const QByteArray key = it.key().toUtf8();
            const QByteArray value = it.value().toUtf8();
            rz_config_set(core->config, key.constData(), value.constData());
        }
    }

    void setConfigi(const QString &config, ut64 val)
    {
        const CutterDiffLocked lock(cutterDiff);
        const RzCore *const core = orig ? lock.coreA : lock.coreB;

        const QByteArray key = config.toUtf8();

        if (!iHash.contains(config)) {
            iHash.insert(config, rz_config_get_i(core->config, key.constData()));
        }

        rz_config_set_i(core->config, key.constData(), val);
    }

    void setConfigb(const QString &config, bool val)
    {
        const CutterDiffLocked lock(cutterDiff);
        const RzCore *const core = orig ? lock.coreA : lock.coreB;

        const QByteArray key = config.toUtf8();

        if (!bHash.contains(config)) {
            bHash.insert(config, rz_config_get_b(core->config, key.constData()));
        }

        rz_config_set_b(core->config, key.constData(), val);
    }

    void setConfig(const QString &config, const QString &val)
    {
        const CutterDiffLocked lock(cutterDiff);
        const RzCore *const core = orig ? lock.coreA : lock.coreB;

        const QByteArray key = config.toUtf8();

        if (!cHash.contains(config)) {
            const char *oldValue = rz_config_get(core->config, key.constData());
            cHash.insert(config, oldValue ? QString::fromUtf8(oldValue) : QString());
        }

        const QByteArray value = val.toUtf8();
        rz_config_set(core->config, key.constData(), value.constData());
    }

private:
    CutterDiff *cutterDiff;
    const bool orig;

    QHash<QString, ut64> iHash;
    QHash<QString, bool> bHash;
    QHash<QString, QString> cHash;
};

#endif // CUTTERDIFF_H
