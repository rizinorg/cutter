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
    // if condition to be put here as well
    result = cutterDiff->matchFunctions(compareLogic, threadCallback, this);
    sortFunctions();
    // //block diffing
    storeBlocksDiff();
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
        CutterDiffItem &item = cutterDiff->diffItemList.emplaceBack(
                DiffItemMatched, entry.fcnA, entry.fcnB,
                RZ_ANALYSIS_SIMILARITY_TYPE_STR(entry.similarity), entry.similarity);
        // condition for dias
        const QString disasA = cutterDiff->disassembleBasicBlock(entry.fcnA->addr, true);
        const QString disasB = cutterDiff->disassembleBasicBlock(entry.fcnB->addr, false);
        item.descA["disas"] = disasA;
        item.descB["disas"] = disasB;
        RzDiff *diff = cutterDiff->lineDiff(disasA, disasB);
        RzList *groups = cutterDiff->lineDiffOpsGrouped(diff);
        item.addLineDiff("disas", diff, groups);
    }

    // Add discarded functions as well
    for (const RzAnalysisFunction *func : discardedA) {
        CutterDiffItem &item =
                cutterDiff->diffItemList.emplaceBack(DiffItemRemoved, func, nullptr, "", 0);
        const QString disasA = cutterDiff->disassembleBasicBlock(func->addr, true);
        // condition for disas
        item.descA["disas"] = disasA;
    }

    const RzAnalysisFunction *func = nullptr;

    CutterRzListForeach (result->unmatch_a, it, RzAnalysisFunction, func) {
        CutterDiffItem &item =
                cutterDiff->diffItemList.emplaceBack(DiffItemRemoved, func, nullptr, "", 0);
        const QString disasA = cutterDiff->disassembleBasicBlock(func->addr, true);
        // condition for disas
        item.descA["disas"] = disasA;
    }
    CutterRzListForeach (result->unmatch_b, it, RzAnalysisFunction, func) {
        CutterDiffItem &item =
                cutterDiff->diffItemList.emplaceBack(DiffItemAdded, nullptr, func, "", 0);
        const QString disasB = cutterDiff->disassembleBasicBlock(func->addr, false);
        item.descB["disas"] = disasB;
    }
    cutterDiff->functionsAnalyzed = true;
}

static int compareBlocks(const RzAnalysisBlock *a, const RzAnalysisBlock *b, void *user)
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

struct MatchBlockEntry
{
    const RzAnalysisBlock *blockA;
    const RzAnalysisBlock *blockB;
    double similarity;
};

void BinDiff::storeBlocksDiff()
{
    RzAnalysisMatchResult *result = nullptr;
    const RzAnalysisMatchPair *pair = nullptr;
    const RzListIter *it = nullptr;
    const RzAnalysisBlock *bb = nullptr;
    for (CutterDiffItem &diffItem : cutterDiff->getDiffItemList()) {
        if (diffItem.getType() != DiffItemMatched) {
            continue;
        }
        const BinDiffMatchDescription match = diffItem.toBinDiffMatchDescription();
        result = cutterDiff->matchFunctionBlocks(match.original.offset, match.modified.offset);
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
            CutterDiffItem &diffBlock = diffItem.blocks.emplaceBack(DiffItemMatched, blockA, blockB,
                                                                    "", entry.similarity);
            // Condition for getting disassembly
            const QString disasA = cutterDiff->disassembleBasicBlock(blockA->addr, true);
            const QString disasB = cutterDiff->disassembleBasicBlock(blockB->addr, false);
            RzDiff *diff = cutterDiff->lineDiff(disasA, disasB);
            RzList *groups = cutterDiff->lineDiffOpsGrouped(diff);
            diffBlock.addLineDiff("disas", diff, groups);
            diffItem.offsetAtoB[blockA->addr] = blockB->addr;
            diffItem.offsetBtoA[blockB->addr] = blockA->addr;
        }

        for (const RzAnalysisBlock *bb : discardedA) {
            CutterDiffItem &diffBlock = diffItem.blocks.emplaceBack(
                    DiffItemRemoved, static_cast<const RzAnalysisBlock *>(bb), nullptr, "", 0);
            diffBlock.descA["disas"] = cutterDiff->disassembleBasicBlock(bb->addr, true);
            // condition for getting disassembly
        }

        CutterRzListForeach (result->unmatch_a, it, RzAnalysisBlock, bb) {
            CutterDiffItem &diffBlock = diffItem.blocks.emplaceBack(
                    DiffItemRemoved, static_cast<const RzAnalysisBlock *>(bb), nullptr, "", 0);
            diffBlock.descA["disas"] = cutterDiff->disassembleBasicBlock(bb->addr, true);
            // condition for getting disassembly
        }
        CutterRzListForeach (result->unmatch_b, it, RzAnalysisBlock, bb) {
            CutterDiffItem &diffBlock = diffItem.blocks.emplaceBack(
                    DiffItemAdded, nullptr, static_cast<const RzAnalysisBlock *>(bb), "", 0);

            // Condition for getting disassembly
            diffBlock.descB["disas"] = cutterDiff->disassembleBasicBlock(bb->addr, false);
        }
    }
    cutterDiff->blocksAnalyzed = true;
    if (result) {
        rz_analysis_match_result_free(result);
    }
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
