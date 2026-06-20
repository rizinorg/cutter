#include "CutterDiff.h"

#include "Configuration.h"

CutterDiff::CutterDiff(QObject *parent) : QObject { parent } {}

CutterDiff::~CutterDiff(){
    rz_core_free(coreA);
    rz_core_free(coreB);
}

bool CutterDiff::initCores(){
    mutex.lock();

    rz_core_free(coreA);
    rz_core_free(coreB);

    coreA = rz_core_new();
    coreB = rz_core_new();

    if(!(coreA||coreB)){
        goto fail;
    }

    rz_core_loadlibs(coreA, RZ_CORE_LOADLIBS_ALL);
    rz_core_loadlibs(coreB, RZ_CORE_LOADLIBS_ALL);

    coreA->print->scr_prompt = false;
    coreB->print->scr_prompt = false;
    mutex.unlock();
    return true;
fail:
    rz_core_free(coreA);
    rz_core_free(coreB);
    mutex.unlock();
    return false;
}

bool CutterDiff::openFiles(const QString &fileA, const QString &fileB){
    mutex.lock();
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
    mutex.unlock();
    return true;
fail:
    rz_core_file_close_all_but(coreA);
    rz_core_file_close_all_but(coreB);
    mutex.unlock();
    return false;
}

void CutterDiff::syncConfig(){
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

bool CutterDiff::analyzeCores(int level){
    mutex.lock();
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
fail:
    mutex.unlock();
    return false;
}

QList<FunctionDescription> CutterDiff::getFunctionList(bool orig)
{
    mutex.lock();
    QList<FunctionDescription> list;
    const RzList *functions = rz_analysis_function_list(orig ? coreA->analysis : coreB->analysis);
    if (!functions) {
        mutex.unlock();
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
    mutex.unlock();
    return list;
}

#define IS_IMPORT(name)                                                                            \
(name.startsWith("sym.imp.") || name.startsWith("loc.imp.") || name.startsWith("imp."))
#define IS_SYMBOL(name, pfx) (IS_IMPORT(name) || name.startsWith(pfx))

RzList *CutterDiff::getFunctions(RzAnalysis *analysis, int compareLogic)
{

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



RzAnalysisMatchResult *CutterDiff::matchFunctions(int compareLogic,RzAnalysisMatchThreadInfoCb callback, void *user)
{
    mutex.lock();
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
