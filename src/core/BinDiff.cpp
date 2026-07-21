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
    cutterDiff.reset(new CutterDiff());
}

BinDiff::~BinDiff()
{
    rz_analysis_match_result_free(result);
}

bool BinDiff::hasData()
{
    return result != nullptr;
}

void BinDiff::setFileA(QString filePath)
{
    mutex.lock();
    fileA = std::move(filePath);
    mutex.unlock();
}

void BinDiff::setFileB(QString filePath)
{
    mutex.lock();
    fileB = std::move(filePath);
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
    continueRun = true;
    maxTotal = 1; // maxTotal must be at least 1.
    mutex.unlock();
    cutterDiff->initCores();
    cutterDiff->syncConfig();
    cutterDiff->openFiles(fileA, fileB);
    cutterDiff->analyzeCores(level);
    result = cutterDiff->matchFunctions(compareLogic, threadCallback, this);
    sortFunctions();
    mutex.lock();
    const bool canComplete = continueRun;
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
    desc->linearSize = rz_analysis_function_linear_size(const_cast<RzAnalysisFunction *>(func));
    desc->nargs = rz_analysis_arg_count(const_cast<RzAnalysisFunction *>(func));
    desc->nlocals = rz_analysis_var_local_count(const_cast<RzAnalysisFunction *>(func));
    desc->nbbs = rz_pvector_len(func->bbs);
    desc->calltype = func->cc ? QString::fromUtf8(func->cc) : QString();
    desc->name = func->name ? QString::fromUtf8(func->name) : QString();
    desc->edges = rz_analysis_function_count_edges(func, nullptr);
    desc->stackframe = func->maxstack;
}

struct MatchEntry
{
    const RzAnalysisFunction *fcnA;
    const RzAnalysisFunction *fcnB;
    double similarity;
};

void BinDiff::sortFunctions()
{
    if (!result) {
        return;
    }

    matchedList.clear();
    removedSet.clear();
    addedSet.clear();

    QHash<const RzAnalysisFunction *, MatchEntry> bestMatches;
    QSet<const RzAnalysisFunction *> discardedA;

    const RzAnalysisMatchPair *pair = nullptr;
    const RzListIter *it = nullptr;

    CutterRzListForeach (result->matches, it, RzAnalysisMatchPair, pair) {

        auto *fcnA = static_cast<const RzAnalysisFunction *>(pair->pair_a);
        auto *fcnB = static_cast<const RzAnalysisFunction *>(pair->pair_b);

        auto hashIt = bestMatches.find(fcnB);

        if (hashIt == bestMatches.end()) {
            bestMatches.insert(fcnB, { fcnA, fcnB, pair->similarity });
            continue;
        }

        if (pair->similarity > hashIt->similarity) {

            discardedA.insert(hashIt->fcnA);

            discardedA.remove(fcnA);

            hashIt->fcnA = fcnA;
            hashIt->similarity = pair->similarity;
        } else {

            discardedA.insert(fcnA);
        }
    }

    for (const auto &entry : std::as_const(bestMatches)) {

        BinDiffMatchDescription desc;

        setFunctionDescription(&desc.original, entry.fcnA);
        setFunctionDescription(&desc.modified, entry.fcnB);

        desc.similarity = entry.similarity;
        desc.simtype = RZ_ANALYSIS_SIMILARITY_TYPE_STR(entry.similarity);

        matchedList.push_back(std::move(desc));
    }
    removedSet = std::move(discardedA);

    const RzAnalysisFunction *func = nullptr;

    CutterRzListForeach (result->unmatch_a, it, RzAnalysisFunction, func) {
        removedSet.insert(func);
    }
    CutterRzListForeach (result->unmatch_b, it, RzAnalysisFunction, func) {
        addedSet.insert(func);
    }
}

QList<BinDiffMatchDescription> BinDiff::matches()
{
    return matchedList;
}

QList<FunctionDescription> BinDiff::mismatch(bool originalFile)
{
    QList<FunctionDescription> list;
    for (const RzAnalysisFunction *func : (originalFile ? removedSet : addedSet)) {
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
    const bool ret = continueRun;
    mutex.unlock();
    return ret;
}
