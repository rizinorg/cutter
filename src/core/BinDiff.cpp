#include "BinDiff.h"

bool BinDiff::threadCallback(const size_t nLeft, const size_t nMatch, void *user)
{
    auto bdiff = reinterpret_cast<BinDiff *>(user);
    return bdiff->updateProgress(nLeft, nMatch);
}

BinDiff::BinDiff(CutterCore *core)
    : core(core),
      result(nullptr),
      continue_run(true),
      maxTotal(1)
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
      ,
      mutex(QMutex::Recursive)
#endif
{
}

BinDiff::~BinDiff()
{
    cancel();
    wait();
    rz_analysis_match_result_free(result);
}

void BinDiff::run(QString fileName)
{
    mutex.lock();
    rz_analysis_match_result_free(result);
    result = nullptr;
    continue_run = true;
    maxTotal = 1; // maxTotal must be at least 1.
    mutex.unlock();

    core->coreMutex.lock();
    result = core->diffNewFile(fileName, threadCallback, this);
    core->coreMutex.unlock();

    emit complete();
}

void BinDiff::cancel()
{
    mutex.lock();
    continue_run = false;
    mutex.unlock();
}

static void setFunctionDescription(FunctionDescription *desc, const RzAnalysisFunction *func)
{
    desc->offset = func->addr;
    desc->linearSize = rz_analysis_function_linear_size((RzAnalysisFunction *)func);
    desc->nargs = rz_analysis_arg_count((RzAnalysisFunction *)func);
    desc->nlocals = rz_analysis_var_local_count((RzAnalysisFunction *)func);
    desc->nbbs = rz_list_length(func->bbs);
    desc->calltype = func->cc ? QString::fromUtf8(func->cc) : QString();
    desc->name = func->name ? QString::fromUtf8(func->name) : QString();
    desc->edges = rz_analysis_function_count_edges(func, nullptr);
    desc->stackframe = func->maxstack;
}

QList<BinDiffMatchDescription> BinDiff::matches()
{
    QList<BinDiffMatchDescription> pairs;
    RzAnalysisMatchPair *pair = nullptr;
    RzListIter *it = nullptr;
    const RzAnalysisFunction *fcn_a = nullptr;
    const RzAnalysisFunction *fcn_b = nullptr;

    CutterRzListForeach (result->matches, it, RzAnalysisMatchPair, pair) {
        BinDiffMatchDescription desc;
        fcn_a = static_cast<const RzAnalysisFunction *>(pair->pair_a);
        fcn_b = static_cast<const RzAnalysisFunction *>(pair->pair_b);

        setFunctionDescription(&desc.original, fcn_a);
        setFunctionDescription(&desc.modified, fcn_b);

        desc.simtype = RZ_ANALYSIS_SIMILARITY_TYPE_STR(pair->similarity);
        desc.similarity = pair->similarity;

        pairs << desc;
    }

    return pairs;
}

QList<FunctionDescription> BinDiff::mismatch(bool fileA)
{
    QList<FunctionDescription> list;
    RzAnalysisFunction *func = nullptr;
    RzList *unmatch = fileA ? result->unmatch_a : result->unmatch_b;
    RzListIter *it = nullptr;

    CutterRzListForeach (unmatch, it, RzAnalysisFunction, func) {
        FunctionDescription desc;
        setFunctionDescription(&desc, func);
        list << desc;
    }

    return list;
}

bool BinDiff::updateProgress(const size_t nLeft, const size_t nMatch)
{
    mutex.lock();

    if (maxTotal < nMatch) {
        maxTotal = nMatch;
    }
    if (maxTotal < nLeft) {
        maxTotal = nLeft;
    }

    emit progress(maxTotal, nLeft, nMatch);
    bool ret = continue_run;
    mutex.unlock();
    return ret;
}
