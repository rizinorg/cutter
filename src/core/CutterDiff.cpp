#include "CutterDiff.h"

#include "Configuration.h"

#include <QMutexLocker>

#define LOCK() const CutterDiffLocked lock(this);

CutterDiff::CutterDiff(QObject *parent)
    : QObject { parent }
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
      ,
      mutex(QMutex::Recursive)
#endif
{
}

CutterDiff::~CutterDiff()
{
    rz_core_free(coreA);
    rz_core_free(coreB);
}

bool CutterDiff::initCores()
{
    LOCK();

    rz_core_free(coreA);
    rz_core_free(coreB);

    coreA = rz_core_new();
    coreB = rz_core_new();

    if (!(coreA || coreB)) {
        goto fail;
    }

    rz_core_loadlibs(coreA, RZ_CORE_LOADLIBS_ALL);
    rz_core_loadlibs(coreB, RZ_CORE_LOADLIBS_ALL);

    coreA->print->scr_prompt = false;
    coreB->print->scr_prompt = false;
    return true;
fail:
    rz_core_free(coreA);
    rz_core_free(coreB);
    return false;
}

bool CutterDiff::openFiles(const QString &fileA, const QString &fileB)
{

    LOCK();
    // open core files
    if (!rz_core_file_open(coreA, fileA.toUtf8().constData(), RZ_PERM_RX, 0)) {
        qWarning() << tr("cannot open file %1").arg(fileA);
        goto fail;
    }

    if (!rz_core_file_open(coreB, fileB.toUtf8().constData(), RZ_PERM_RX, 0)) {
        qWarning() << tr("cannot open file %1").arg(fileB);
        goto fail;
    }

    if (!rz_core_bin_load(coreA, nullptr, UT64_MAX)) {
        qWarning() << tr("cannot load bin %1").arg(fileA);
        goto fail;
    }

    if (!rz_core_bin_load(coreB, nullptr, UT64_MAX)) {
        qWarning() << tr("cannot load bin %1").arg(fileB);
        goto fail;
    }

    if (!rz_core_bin_update_arch_bits(coreA)) {
        qWarning() << tr("cannot set architecture with bits in fileA");
        goto fail;
    }

    if (!rz_core_bin_update_arch_bits(coreB)) {
        qWarning() << tr("cannot set architecture with bits in fileB");
        goto fail;
    }
    syncConfig();
    return true;
fail:
    rz_core_file_close_all_but(coreA);
    rz_core_file_close_all_but(coreB);
    return false;
}

void CutterDiff::syncConfig()
{
    RzConfigEntry *var;
    RzConfigNode *node;
    RzCoreLocked core(Core());
    CutterRzVectorForeach(&core->config->sorted_vars, var, RzConfigEntry)
    {
        node = &var->node;
        if (!strcmp(node->name, "scr.color") || !strcmp(node->name, "scr.interactive")
            || !strcmp(node->name, "cfg.debug")) {
            rz_config_set(coreA->config, node->name, "0");
            rz_config_set(coreB->config, node->name, "0");
            continue;
        }
        rz_config_set(coreA->config, node->name, node->value);
        rz_config_set(coreB->config, node->name, node->value);
    }
}

bool CutterDiff::analyzeCores(int level)
{
    LOCK();
    if (!rz_core_analysis_all(coreA)) {
        qWarning() << tr("cannot perform basic analysis of the binary fileA");
        goto fail;
    }
    if (!rz_core_analysis_all(coreB)) {
        qWarning() << tr("cannot perform basic analysis of the binary fileB");
        goto fail;
    }

    if (level != AnalysisLevelSymbols
        && !rz_core_analysis_everything(coreA, level == AnalysisLevelExperimental, nullptr)) {
        qWarning() << tr("cannot perform complete analysis of the binaryA");
        goto fail;
    }

    if (level != AnalysisLevelSymbols
        && !rz_core_analysis_everything(coreB, level == AnalysisLevelExperimental, nullptr)) {
        qWarning() << tr("cannot perform complete analysis of the binaryB");
        goto fail;
    }
    return true;
fail:
    return false;
}

QList<FunctionDescription> CutterDiff::getFunctionList(bool orig)
{
    LOCK();
    QList<FunctionDescription> list;
    const RzList *functions = rz_analysis_function_list(orig ? coreA->analysis : coreB->analysis);
    if (!functions) {
        return list;
    }
    RzAnalysisFunction *func = nullptr;
    const RzListIter *it = nullptr;
    CutterRzListForeach (functions, it, RzAnalysisFunction, func) {
        FunctionDescription desc;
        desc.offset = func->addr;
        desc.linearSize = rz_analysis_function_linear_size(const_cast<RzAnalysisFunction *>(func));
        desc.nargs = rz_analysis_arg_count(const_cast<RzAnalysisFunction *>(func));
        desc.nlocals = rz_analysis_var_local_count(const_cast<RzAnalysisFunction *>(func));
        desc.nbbs = rz_pvector_len(func->bbs);
        desc.calltype = func->cc ? QString::fromUtf8(func->cc) : QString();
        desc.name = func->name ? QString::fromUtf8(func->name) : QString();
        desc.edges = rz_analysis_function_count_edges(func, nullptr);
        desc.stackframe = func->maxstack;
        list.push_back(desc);
    }
    return list;
}

#define IS_IMPORT(name)                                                                            \
    (name.startsWith("sym.imp.") || name.startsWith("loc.imp.") || name.startsWith("imp."))
#define IS_SYMBOL(name, pfx) (IS_IMPORT(name) || name.startsWith(pfx))

RzList *CutterDiff::getFunctions(RzAnalysis *analysis, int compareLogic)
{
    LOCK();
    const RzList *functions = rz_analysis_function_list(analysis);
    if (!functions) {
        return nullptr;
    }

    RzList *list = rz_list_newf(nullptr);
    if (!list) {
        return list;
    }

    QString pfx = Config()->getConfigString("analysis.fcnprefix");
    if (pfx.isEmpty()) {
        pfx = "fcn.";
    } else {
        pfx += ".";
    }

    RzAnalysisFunction *func = nullptr;
    const RzListIter *it = nullptr;

    CutterRzListForeach (functions, it, RzAnalysisFunction, func) {
        const QString name = func->name;
        if (compareLogic == CutterDiff::CompareLogicDefault && IS_IMPORT(name)) {
            continue;
        } else if (compareLogic == CutterDiff::CompareLogicSymbols && IS_SYMBOL(name, pfx)) {
            continue;
        }
        rz_list_add_sorted(list, func, rz_analysis_get_column_sort(analysis), nullptr);
    }

    return list;
}

QByteArray CutterDiff::ioRead(RVA addr, int len, bool orig)
{
    LOCK();
    const RzCore *core = orig ? coreA : coreB;
    QByteArray array;

    if (len <= 0) {
        return array;
    }

    /* Zero-copy */
    array.resize(len);
    if (!core || !core->io
        || !rz_io_read_at_mapped(core->io, addr, reinterpret_cast<ut8 *>(array.data()), len)) {
        array.fill(0xff);
    }

    return array;
}

RzAnalysisMatchResult *CutterDiff::matchFunctions(int compareLogic,
                                                  RzAnalysisMatchThreadInfoCb callback, void *user)
{
    LOCK();
    RzList *fcnsA = nullptr, *fcnsB = nullptr;
    RzAnalysisMatchResult *result = nullptr;
    RzAnalysisMatchOpt opts;

    fcnsA = getFunctions(coreA->analysis, compareLogic);
    if (rz_list_empty(fcnsA)) {
        qWarning() << tr("no functions found in the current opened file");
        goto fail;
    }

    fcnsB = getFunctions(coreB->analysis, compareLogic);
    if (rz_list_empty(fcnsB)) {
        qWarning() << tr("no functions found in the just opene file %1");
        goto fail;
    }

    opts.analysis_a = coreA->analysis;
    opts.analysis_b = coreB->analysis;
    opts.callback = callback;
    opts.user = user;

    // calculate all the matches between the functions of the 2 different core files.
    result = rz_analysis_match_functions(fcnsA, fcnsB, &opts);
    if (!result) {
        qWarning() << tr("failed to perform the function matching operation or job was cancelled.");
        goto fail;
    }

    rz_list_free(fcnsA);
    rz_list_free(fcnsB);
    return result;

fail:

    rz_list_free(fcnsA);
    rz_list_free(fcnsB);
    // rz_core_file_close_all_but(diffCore);
    return nullptr;
}

QString CutterDiff::getCommentAt(RVA addr, bool orig)
{
    LOCK();
    const RzCore *core = orig ? coreA : coreB;
    if (!core->analysis) {
        return "";
    }
    return rz_meta_get_string(core->analysis, RZ_META_TYPE_COMMENT, addr);
}

QString CutterDiff::listFlagsAsStringAt(RVA addr, bool orig)
{
    LOCK();
    const RzCore *core = orig ? coreA : coreB;
    if (!core || !core->flags) {
        return "";
    }
    char *flagList = rz_flag_get_liststr(core->flags, addr);
    QString result = fromOwnedCharPtr(flagList);
    return result;
}

bool CutterDiff::cmpBytesAt(RVA addrA, RVA addrB, size_t len)
{
    LOCK();
    if (len <= 0) {
        return true;
    }
    if (!coreA || !coreB || !coreA->io || !coreB->io) {
        qWarning() << "Cores not initialized";
        return false;
    }
    QByteArray bufA;
    bufA.resize(len);
    QByteArray bufB;
    bufB.resize(len);

    if (!rz_io_read_at_mapped(coreA->io, addrA, reinterpret_cast<ut8 *>(bufA.data()), len)
        || !rz_io_read_at_mapped(coreB->io, addrB, reinterpret_cast<ut8 *>(bufB.data()), len)) {
        qWarning() << "Read failed";
        return false;
    };
    return bufA == bufB;
}

QString CutterDiff::getCommonConfig(const char *k)
{
    LOCK();
    return { rz_config_get(coreA->config, k) };
}

void CutterDiff::setCommonConfig(const char *k, const char *v)
{
    LOCK();
    rz_config_set(coreA->config, k, v);
    rz_config_set(coreB->config, k, v);
}

int CutterDiff::getCommonConfigi(const char *k)
{
    LOCK();
    return static_cast<int>(rz_config_get_i(coreA->config, k));
}

bool CutterDiff::getCommonConfigb(const char *k)
{
    LOCK();
    return rz_config_get_b(coreA->config, k);
}

void CutterDiff::setCommonConfigb(const char *k, bool value)
{
    LOCK();
    rz_config_set_b(coreA->config, k, value);
    rz_config_set_b(coreB->config, k, value);
}

void CutterDiff::setCommonConfigi(const char *k, int v)
{
    LOCK();
    rz_config_set_i(coreA->config, k, static_cast<ut64>(v));
    rz_config_set_i(coreB->config, k, static_cast<ut64>(v));
}

void CutterDiff::seekSilent(ut64 offset, bool orig)
{
    LOCK();
    if (offset == RVA_INVALID) {
        return;
    }
    rz_core_seek(orig ? coreA : coreB, offset, true);
}

QString CutterDiff::cmdRawAt(const char *cmd, RVA address, bool orig)
{
    LOCK();
    QString res;
    const RVA oldOffset = getOffset(orig);
    seekSilent(address, orig);

    res = cmdRaw(cmd, orig);

    seekSilent(oldOffset, orig);
    return res;
}

QString CutterDiff::cmdRaw(const char *cmd, bool orig)
{
    LOCK();
    const QString res;
    return rz_core_cmd_str(orig ? coreA : coreB, cmd);
}

RVA CutterDiff::getOffset(bool orig)
{
    return (orig ? coreA : coreB)->offset;
}
