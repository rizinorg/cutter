#include "BinDiff.h"

#include <QMutexLocker>

FunctionMatchJob::FunctionMatchJob(CutterDiff *cutterDiff, const BinDiffOptions &options,
                                   QObject *parent)
    : BinDiffJob(cutterDiff, options, parent)
{
}

bool FunctionMatchJob::run()
{
    RzAnalysisMatchResult *result =
            cutterDiff->matchFunctions(options.compareLogic, threadCallback, this);
    if (!result) {
        return false;
    }

    QHash<const RzAnalysisFunction *, MatchEntry> bestMatches;
    QSet<const RzAnalysisFunction *> discardedA;

    const RzAnalysisMatchPair *pair = nullptr;
    const RzListIter *it = nullptr;

    // Finding the best match while discarding others
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
        CutterDiffItem &item = cutterDiff->diffItemList.emplace_back(
                DiffItemMatched, entry.fcnA, entry.fcnB,
                RZ_ANALYSIS_SIMILARITY_TYPE_STR(entry.similarity), entry.similarity);
        // condition for dias
        const QString disasA = cutterDiff->disassembleBasicBlock(entry.fcnA->addr, true);
        const QString disasB = cutterDiff->disassembleBasicBlock(entry.fcnB->addr, false);
        item.descA["disas"] = disasA;
        item.descB["disas"] = disasB;
        RzDiff *diff = cutterDiff->lineDiff(disasA, disasB);
        RzList *groups = cutterDiff->lineDiffOpsGrouped(diff);
        item.instrDiffs["disas"] = cutterDiff->rzDiffOpToCutterInstrs(diff, groups);
        rz_diff_free(diff);
        rz_list_free(groups);
    }

    // Add discarded functions as well
    for (const RzAnalysisFunction *func : discardedA) {
        CutterDiffItem &item =
                cutterDiff->diffItemList.emplace_back(DiffItemRemoved, func, nullptr, "", 0);
        const QString disasA = cutterDiff->disassembleBasicBlock(func->addr, true);
        // condition for disas
        item.descA["disas"] = disasA;
    }

    const RzAnalysisFunction *func = nullptr;

    CutterRzListForeach (result->unmatch_a, it, RzAnalysisFunction, func) {
        CutterDiffItem &item =
                cutterDiff->diffItemList.emplace_back(DiffItemRemoved, func, nullptr, "", 0);
        const QString disasA = cutterDiff->disassembleBasicBlock(func->addr, true);
        // condition for disas
        item.descA["disas"] = disasA;
    }
    CutterRzListForeach (result->unmatch_b, it, RzAnalysisFunction, func) {
        CutterDiffItem &item =
                cutterDiff->diffItemList.emplace_back(DiffItemAdded, nullptr, func, "", 0);
        const QString disasB = cutterDiff->disassembleBasicBlock(func->addr, false);
        item.descB["disas"] = disasB;
    }
    cutterDiff->functionsAnalyzed = true;
    rz_analysis_match_result_free(result);
    return true;
}

bool FunctionMatchJob::threadCallback(const size_t nLeft, const size_t nMatches, void *user)
{
    auto fMJob = reinterpret_cast<FunctionMatchJob *>(user);
    return fMJob->validateProgress(nLeft, nMatches);
}

bool FunctionMatchJob::validateProgress(const size_t nLeft, const size_t nMatches)
{
    if (nMatches > maxTotal) {
        maxTotal = nMatches;
    }
    if (nLeft > maxTotal) {
        maxTotal = nLeft;
    }
    updateProgress(double(maxTotal - nLeft) / double(maxTotal));
    return canContinue;
}

BlocksMatchJob::BlocksMatchJob(CutterDiff *cutterDiff, const BinDiffOptions &options,
                               QObject *parent)
    : BinDiffJob(cutterDiff, options, parent)
{
}

static int compareBlocks(const RzAnalysisBlock *a, const RzAnalysisBlock *b, void * /*user*/)
{
    return (a && b && a->addr && b->addr ? (a->addr > b->addr) - (a->addr < b->addr) : 0);
}

static int comparePairBlocks(const RzAnalysisMatchPair *ma, const RzAnalysisMatchPair *mb,
                             void *user)
{
    auto *a = static_cast<const RzAnalysisBlock *>(ma->pair_a);
    auto *b = static_cast<const RzAnalysisBlock *>(mb->pair_a);
    return compareBlocks(a, b, user);
}

bool BlocksMatchJob::run()
{
    RzAnalysisMatchResult *result = nullptr;
    const RzAnalysisMatchPair *pair = nullptr;
    const RzListIter *it = nullptr;
    const RzAnalysisBlock *bb = nullptr;
    for (CutterDiffItem &diffItem : cutterDiff->getDiffItemList()) {
        maxTotal = 1;
        if (diffItem.getType() == DiffItemMatched) {
            const BinDiffMatchDescription match = diffItem.toBinDiffMatchDescription();
            result = cutterDiff->matchFunctionBlocks(match.original.offset, match.modified.offset,
                                                     threadCallback, this);
            if (!result) {
                qWarning() << "Failed to perform blocks matching";
                continue;
            }
            rz_list_sort(result->matches, reinterpret_cast<RzListComparator>(comparePairBlocks),
                         nullptr);
            rz_list_sort(result->unmatch_a, reinterpret_cast<RzListComparator>(comparePairBlocks),
                         nullptr);
            rz_list_sort(result->unmatch_b, reinterpret_cast<RzListComparator>(comparePairBlocks),
                         nullptr);
            // Find Best Pairs
            QHash<const RzAnalysisBlock *, MatchBlockEntry> bestMatches;
            QSet<const RzAnalysisBlock *> discardedA;

            CutterRzListForeach (result->matches, it, RzAnalysisMatchPair, pair) {
                auto *blockA = static_cast<const RzAnalysisBlock *>(pair->pair_a);
                auto *blockB = static_cast<const RzAnalysisBlock *>(pair->pair_b);

                auto hashIt = bestMatches.find(blockB);

                if (hashIt == bestMatches.end()) {
                    bestMatches.insert(blockB, { blockA, blockB, pair->similarity });
                    continue;
                }

                if (pair->similarity > hashIt->similarity) {

                    discardedA.insert(hashIt->blockA);

                    discardedA.remove(blockA);

                    hashIt->blockA = blockA;
                    hashIt->similarity = pair->similarity;
                } else {

                    discardedA.insert(blockA);
                }
            }

            for (const MatchBlockEntry &entry : std::as_const(bestMatches)) {
                const RzAnalysisBlock *blockA = entry.blockA;
                const RzAnalysisBlock *blockB = entry.blockB;
                CutterDiffItem &diffBlock = diffItem.blocks.emplace_back(
                        DiffItemMatched, blockA, blockB, "", entry.similarity);
                // Condition for getting disassembly
                const QString disasA = cutterDiff->disassembleBasicBlock(blockA->addr, true);
                const QString disasB = cutterDiff->disassembleBasicBlock(blockB->addr, false);
                RzDiff *diff = cutterDiff->lineDiff(disasA, disasB);
                RzList *groups = cutterDiff->lineDiffOpsGrouped(diff);
                diffBlock.instrDiffs["disas"] = cutterDiff->rzDiffOpToCutterInstrs(diff, groups);
                diffItem.offsetAtoB[blockA->addr] = blockB->addr;
                diffItem.offsetBtoA[blockB->addr] = blockA->addr;
                rz_diff_free(diff);
                rz_list_free(groups);
            }

            for (const RzAnalysisBlock *bb : discardedA) {
                CutterDiffItem &diffBlock = diffItem.blocks.emplace_back(
                        DiffItemRemoved, static_cast<const RzAnalysisBlock *>(bb), nullptr, "", 0);
                diffBlock.descA["disas"] = cutterDiff->disassembleBasicBlock(bb->addr, true);
                // condition for getting disassembly
            }

            CutterRzListForeach (result->unmatch_a, it, RzAnalysisBlock, bb) {
                CutterDiffItem &diffBlock = diffItem.blocks.emplace_back(
                        DiffItemRemoved, static_cast<const RzAnalysisBlock *>(bb), nullptr, "", 0);
                diffBlock.descA["disas"] = cutterDiff->disassembleBasicBlock(bb->addr, true);
                // condition for getting disassembly
            }
            CutterRzListForeach (result->unmatch_b, it, RzAnalysisBlock, bb) {
                CutterDiffItem &diffBlock = diffItem.blocks.emplace_back(
                        DiffItemAdded, nullptr, static_cast<const RzAnalysisBlock *>(bb), "", 0);

                // Condition for getting disassembly
                diffBlock.descB["disas"] = cutterDiff->disassembleBasicBlock(bb->addr, false);
            }
            rz_analysis_match_result_free(result);
            result = nullptr;
        } else if (diffItem.getType() == DiffItemRemoved) {
            const RzAnalysisFunction *func = rz_analysis_get_function_at(
                    cutterDiff->coreA->analysis, diffItem.descriptionA()["offset"].toULongLong());
            if (!func) {
                continue;
            }
            for (auto *bb : CutterPVector<RzAnalysisBlock>(func->bbs)) {
                CutterDiffItem &diffBlock = diffItem.blocks.emplace_back(
                        DiffItemRemoved, static_cast<const RzAnalysisBlock *>(bb), nullptr, "", 0);
                diffBlock.descA["disas"] = cutterDiff->disassembleBasicBlock(bb->addr, true);
            }
        } else {
            const RzAnalysisFunction *func = rz_analysis_get_function_at(
                    cutterDiff->coreB->analysis, diffItem.descriptionB()["offset"].toULongLong());
            if (!func) {
                continue;
            }
            for (auto *bb : CutterPVector<RzAnalysisBlock>(func->bbs)) {
                CutterDiffItem &diffBlock = diffItem.blocks.emplace_back(
                        DiffItemAdded, nullptr, static_cast<const RzAnalysisBlock *>(bb), "", 0);
                diffBlock.descB["disas"] = cutterDiff->disassembleBasicBlock(bb->addr, false);
            }
        }
        parsedFunctions++;
    }
    cutterDiff->blocksAnalyzed = true;
    return true;
}

bool BlocksMatchJob::threadCallback(const size_t nLeft, const size_t nMatches, void *user)
{
    auto fMJob = reinterpret_cast<BlocksMatchJob *>(user);
    return fMJob->validateProgress(nLeft, nMatches);
}

bool BlocksMatchJob::validateProgress(const size_t nLeft, const size_t nMatches)
{
    if (nMatches > maxTotal) {
        maxTotal = nMatches;
    }
    if (nLeft > maxTotal) {
        maxTotal = nLeft;
    }
    updateProgress((double(parsedFunctions) + double(maxTotal - nLeft) / double(maxTotal))
                   / double(cutterDiff->getDiffItemList().size()));
    return canContinue;
}

BinDiff::BinDiff(CutterDiff *cutterDiff, const BinDiffOptions &options)
    : options(options),
      cutterDiff(cutterDiff),
      continueRun(true),
      maxTotal(1)
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
      ,
      mutex(QMutex::Recursive)
#endif
{
    Q_ASSERT(cutterDiff != nullptr);

    // process and add jobs based on the options
    diffJobs.push_back(new CutterDiffInitJob(cutterDiff, options, this));
    diffJobs.push_back(new FunctionMatchJob(cutterDiff, options, this));
    diffJobs.push_back(new BlocksMatchJob(cutterDiff, options, this));
}

BinDiff::~BinDiff() {}

void BinDiff::run()
{
    qRegisterMetaType<BinDiffStatusDescription>();
    // To prevent read write error between jobs
    const QMutexLocker locker(&cutterDiff->analysisMutex);
    for (BinDiffJob *job : diffJobs) {
        mutex.lock();
        currentJob = job;
        mutex.unlock();
        const QMetaObject::Connection connection =
                connect(job, &BinDiffJob::progressUpdated, this, &BinDiff::updateProgress);
        const bool success = job->run();
        disconnect(connection);
        if (!success || !continueRun) {
            qWarning() << "Failed to successfully complete job: " << job->name();
            break;
        }
        mutex.lock();
        completedJobs++;
        mutex.unlock();
        qInfo() << completedJobs;
    }
    mutex.lock();
    const bool canComplete = continueRun;
    mutex.unlock();
    if (canComplete) {
        emit complete();
    }
    cutterDiff->emitUpdate();
}

void BinDiff::cancel()
{
    mutex.lock();
    continueRun = false;
    if (currentJob) {
        currentJob->cancel();
    }
    mutex.unlock();
}

bool BinDiff::updateProgress(double currentProgress)
{
    BinDiffStatusDescription status;
    bool ret;

    {
        const QMutexLocker locker(&mutex);
        status.total = diffJobs.size();
        status.nLeft = double(completedJobs) + std::clamp(currentProgress, 0.0, 1.0);
        status.nMatch = 0;
        ret = continueRun;
    }

    emit progress(status);

    return ret;
}
