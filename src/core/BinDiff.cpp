#include "BinDiff.h"

bool BinDiff::threadCallback(const size_t nLeft, const size_t nMatch, void *user)
{
    auto bdiff = reinterpret_cast<BinDiff *>(user);
    return bdiff->updateProgress(nLeft, nMatch);
}

BinDiff::BinDiff()
    : result(nullptr),
      continueRun(true),
      maxTotal(1)
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
      ,
      mutex(QMutex::Recursive)
#endif
{
}

BinDiff::~BinDiff()
{
    rz_analysis_match_result_free(result);
}

bool BinDiff::hasData()
{
    return result != nullptr;
}

void BinDiff::setFile(QString filePath)
{
    mutex.lock();
    file = filePath;
    mutex.unlock();
}

void BinDiff::setAnalysisLevel(int aLevel)
{
    mutex.lock();
    level = aLevel;
    mutex.unlock();
}

void BinDiff::setCompareLogic(int cLogic)
{
    mutex.lock();
    compareLogic = cLogic;
    mutex.unlock();
}

void BinDiff::run()
{
    qRegisterMetaType<BinDiffStatusDescription>();

    mutex.lock();
    rz_analysis_match_result_free(result);
    result = nullptr;
    continueRun = true;
    maxTotal = 1; // maxTotal must be at least 1.
    mutex.unlock();

    result = Core()->diffNewFile(file, level, compareLogic, threadCallback, this);

    mutex.lock();
    bool canComplete = continueRun;
    mutex.unlock();
    if (canComplete) {
        emit complete();
    }
}

void BinDiff::cancel()
{
    mutex.lock();
    continueRun = false;
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

    if (!result) {
        return pairs;
    }

    CutterRzListForeach (result->matches, it, RzAnalysisMatchPair, pair) {
        BinDiffMatchDescription desc;
        fcn_a = static_cast<const RzAnalysisFunction *>(pair->pair_a);
        fcn_b = static_cast<const RzAnalysisFunction *>(pair->pair_b);

        setFunctionDescription(&desc.original, fcn_a);
        setFunctionDescription(&desc.modified, fcn_b);

        desc.simtype = RZ_ANALYSIS_SIMILARITY_TYPE_STR(pair->similarity);
        desc.similarity = pair->similarity;

        pairs.push_back(desc);
    }

    return pairs;
}

QList<FunctionDescription> BinDiff::mismatch(bool originalFile)
{
    QList<FunctionDescription> list;
    if (!result) {
        return list;
    }

    RzAnalysisFunction *func = nullptr;
    RzList *unmatch = originalFile ? result->unmatch_a : result->unmatch_b;
    RzListIter *it = nullptr;

    CutterRzListForeach (unmatch, it, RzAnalysisFunction, func) {
        FunctionDescription desc;
        setFunctionDescription(&desc, func);
        list.push_back(desc);
    }

    return list;
}

bool BinDiff::updateProgress(size_t nLeft, size_t nMatch)
{
    mutex.lock();
    if (nMatch > maxTotal) {
        maxTotal = nMatch;
    }
    if (nLeft > maxTotal) {
        maxTotal = nLeft;
    }

    BinDiffStatusDescription status;
    status.total = maxTotal;
    status.nLeft = nLeft;
    status.nMatch = nMatch;

    emit progress(status);
    bool ret = continueRun;
    mutex.unlock();
    return ret;
}
