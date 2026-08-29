#ifndef CUTTERDIFF_H
#define CUTTERDIFF_H

#include "Cutter.h"
#include "CutterCommon.h"
#include "RizinCpp.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QMutex>
#include <QMutexLocker>
#include <QObject>

class CutterDiffLocked;
class CutterDiff;
using CutterDiffItemDescription = QVariantMap;

struct Bound
{
    int pos;
    int size;
};

enum DiffInstrType : ut8 { DiffInstrEqual, DiffInstrReplaced, DiffInstrInserted, DiffInstrDeleted };
struct DiffInstr
{
    QString a;
    QString b;
    DiffInstrType type;
    Bound bound;
};

class CutterDiffItem
{
    friend class BinDiff;

public:
    explicit CutterDiffItem(DiffItemType type, const RzAnalysisFunction *a,
                            const RzAnalysisFunction *b, const QString &simtype = {},
                            double similarity = 0.0);

    explicit CutterDiffItem(DiffItemType type, const RzAnalysisBlock *a, const RzAnalysisBlock *b,
                            const QString &simtype = {}, double similarity = 0.0);
    CutterDiffItem() {} // invalid CutterDiffItem
    ~CutterDiffItem();

    DiffItemType getType() const;

    const CutterDiffItemDescription &descriptionA() const;

    const CutterDiffItemDescription &descriptionB() const;

    const QString &getSimtype() const;

    double getSimilarity() const;
    static FunctionDescription toFunctionDescription(const QVariantMap &desc);
    FunctionDescription functionA() const;

    FunctionDescription functionB() const;
    BinDiffMatchDescription toBinDiffMatchDescription() const;
    bool isFunction() const { return functionDiff; }
    bool isBlock() const { return blockDiff; }
    RVA mapOffset(RVA offset, bool original) const;

    const QList<CutterDiffItem> &getBlocks() const { return blocks; }

    const QHash<QString, QList<DiffInstr>> &getInstrDiffs() const { return instrDiffs; }

    QJsonObject toJson() const;
    static CutterDiffItem fromJson(const QJsonObject &json);

private:
    static CutterDiffItemDescription functionDescription(const RzAnalysisFunction *func);

    static CutterDiffItemDescription blockDescription(const RzAnalysisBlock *bb);
    static bool isValidPair(DiffItemType type, const void *a, const void *b);

private:
    bool functionDiff;
    bool blockDiff;
    DiffItemType type;

    CutterDiffItemDescription descA;
    CutterDiffItemDescription descB;

    QHash<qulonglong, qulonglong> offsetAtoB;
    QHash<qulonglong, qulonglong> offsetBtoA;

    QString simtype;
    double similarity = 0.0;

    QList<CutterDiffItem> blocks;
    QHash<QString, QList<DiffInstr>> instrDiffs;
};

static CutterDiffItem invalidCutterDiffItem;

class CUTTER_EXPORT CutterDiff : public QObject
{
    Q_OBJECT
    friend class CutterDiffLocked;
    friend class BinDiff;

public:
    explicit CutterDiff(QObject *parent = nullptr);
    ~CutterDiff();
    bool initCores();
    bool openFiles(const QString &fileA, const QString &fileB);
    bool analyzeCores(int level);
    void syncConfig();

    enum : ut8 { AnalysisLevelSymbols, AnalysisLevelAuto, AnalysisLevelExperimental };

    enum : ut8 {
        CompareLogicDefault = 0, ///< Only symbols and functions (no imports)
        CompareLogicComplete, ///< All functions (imports included)
        CompareLogicSymbols, ///< Only symbols
    };

    // Sidebar
    QList<FunctionDescription> getFunctionList(bool orig = true);

    // HexDiff
    QByteArray ioRead(RVA addr, int len, bool orig = true);
    QString getCommentAt(RVA addr, bool orig = true);
    QString listFlagsAsStringAt(RVA addr, bool orig = true);
    bool cmpBytesAt(RVA addrA, RVA addrB, size_t len); // unused

    // Assuming both of the cores will be having same config all the time.
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
    QString disassembleFunction(RVA addr, bool orig);
    QString disassembleBasicBlock(RVA addr, bool orig);
    QString ansiEscapeToHtml(const QString &text);
    QString getFileName(bool orig = true) const { return orig ? fileNameA : fileNameB; }
    QString getFilePath(bool orig = true) const { return orig ? filePathA : filePathB; }
    Bound getLineDiffBounds(const QString &line1, const QString &line2);
    bool isFunctionsAnalyzed() const { return functionsAnalyzed; }
    bool isBlocksAnalyzed() const { return blocksAnalyzed; }

    /**
     * @brief getDiffItemList
     * @return BinDiffMatchDescription to display in function similarity table.
     */
    BinDiffMatchDescription getCurrentMatchDescription();

    /**
     * @brief getDiffItemList
     * @return Reference to CutterDiffItem list.
     */
    QList<CutterDiffItem> &getDiffItemList() { return diffItemList; }

    /**
     * @brief setCurrentDiffItemIndex
     * sets currentDiffItemIndex to -1 if out of currentDiffItemIndex is given
     * @param index
     */
    void setCurrentDiffItemIndex(qsizetype index)
    {
        if (index > diffItemList.size()) {
            index = diffItemList.size() - 1;
        }
        if (index < 0) {
            index = -1;
        }
        currentDiffItemIndex = index;
        emit currentItemDiffChanged();
    }

    qsizetype getCurrentDiffItemIndex() { return getCurrentDiffItemIndex(); }

    bool diffEmpty() { return diffItemList.isEmpty(); }

    /**
     * @brief getCurrentDiffItem :returns reference to the current CutterDiffItem
     * returns invalid item if index is invalid
     * @return
     */
    const CutterDiffItem &getCurrentDiffItem() const
    {
        if (currentDiffItemIndex < 0 || currentDiffItemIndex >= diffItemList.size()) {
            return invalidCutterDiffItem;
        }
        return diffItemList[currentDiffItemIndex];
    }

    /**
     * @brief Exporting CutterDiffItems to JSON
     * @param filePath
     * @return Operation success bool
     */
    bool saveDiffItemsToJson(const QString &filePath) const;

    /**
     * @brief loadDiffItemsFromJson Import JSON and store it into CutterDiff
     * @param filePath
     * @return operationSuccessfull bool
     */
    bool loadDiffItemsFromJson(const QString &filePath);

    // maybe adddiffItem
    // removeDiffItem
    // itemupdate signal from diffItems as well which will again trigger dataupdated

    void emitUpdate()
    {
        emit diffDataUpdated();
        qInfo() << "dataUpdated";
    }

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
    bool blocksAnalyzed;
    bool functionsAnalyzed;
    qsizetype currentDiffItemIndex;
    QList<CutterDiffItem> diffItemList;

    // Function pairing
    RZ_OWN RzList *getFunctions(RzAnalysis *analysis, int compareLogic);
    RZ_OWN RzAnalysisMatchResult *matchFunctionBlocks(RVA addrA, RVA addrB);
    RZ_OWN RzAnalysisMatchResult *matchFunctions(int compareLogic,
                                                 RzAnalysisMatchThreadInfoCb callback, void *user);

    // LineDiffing
    RZ_OWN RzDiff *lineDiff(const char *lines1, const char *lines2);
    RZ_OWN RzDiff *lineDiff(const QString &lines1, const QString &lines2);
    RZ_OWN RzDiff *diffFunctionDissas(RVA addrA, RVA addrB);
    RZ_OWN RzDiff *diffBlockDisas(RVA addrA, RVA addrB);
    RZ_OWN RzList *lineDiffOpsGrouped(RzDiff *diff) const;
signals:
    void currentMatchChanged();
    void currentItemDiffChanged();
    void diffDataUpdated();
    // void diffDataUpdated();//data update shall be added to every widget TODO
    // so we can do selective diffing of functions and chosse which all the blocks to be diffed
    // also there shall be a individual Diffing thread like BinDiff for performing diffing on
    // induvidual diffItems without restrcting/obstructing the usability of CutterDiffWindow
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
