#include "CutterDiff.h"

#include "Configuration.h"

#include <QMutexLocker>

#define LOCK() const CutterDiffLocked lock(this);

CutterDiffItem::CutterDiffItem(DiffItemType type, const RzAnalysisFunction *a,
                               const RzAnalysisFunction *b, const QString &simtype,
                               double similarity)
    : type(type), simtype(simtype), similarity(similarity), functionDiff(true), blockDiff(false)
{
    Q_ASSERT(isValidPair(type, a, b));

    if (a) {
        descA = functionDescription(a);
    }

    if (b) {
        descB = functionDescription(b);
    }
}

CutterDiffItem::CutterDiffItem(DiffItemType type, const RzAnalysisBlock *a,
                               const RzAnalysisBlock *b, const QString &simtype, double similarity)
    : type(type), simtype(simtype), similarity(similarity), functionDiff(false), blockDiff(true)
{
    Q_ASSERT(isValidPair(type, a, b));

    if (a) {
        descA = blockDescription(a);
    }

    if (b) {
        descB = blockDescription(b);
    }
}

CutterDiffItem::~CutterDiffItem() {}

DiffItemType CutterDiffItem::getType() const
{
    return type;
}

const CutterDiffItemDescription &CutterDiffItem::descriptionA() const
{
    return descA;
}

const CutterDiffItemDescription &CutterDiffItem::descriptionB() const
{
    return descB;
}

const QString &CutterDiffItem::getSimtype() const
{
    return simtype;
}

double CutterDiffItem::getSimilarity() const
{
    return similarity;
}
FunctionDescription CutterDiffItem::toFunctionDescription(const QVariantMap &desc)
{
    FunctionDescription f;
    f.offset = desc["offset"].toULongLong();
    f.linearSize = desc["linearSize"].toULongLong();
    f.nargs = desc["nargs"].toULongLong();
    f.nbbs = desc["nbbs"].toULongLong();
    f.nlocals = desc["nlocals"].toULongLong();
    f.calltype = desc["calltype"].toString();
    f.name = desc["name"].toString();
    f.edges = desc["edges"].toULongLong();
    f.stackframe = desc["stackframe"].toULongLong();
    f.disas = desc["disas"].toStringList();
    return f;
}

FunctionDescription CutterDiffItem::functionA() const
{
    if (type == DiffItemAdded) {
        return {};
    }
    return toFunctionDescription(descA);
}

FunctionDescription CutterDiffItem::functionB() const
{
    if (type == DiffItemRemoved) {
        return {};
    }
    return toFunctionDescription(descB);
}
BinDiffMatchDescription CutterDiffItem::toBinDiffMatchDescription() const
{
    return { functionA(), functionB(), simtype, similarity };
}

RVA CutterDiffItem::mapOffset(RVA offset, bool original) const
{
    if (type != DiffItemMatched) {
        return RVA_INVALID;
    }
    const QHash<qulonglong, qulonglong> &mappings = original ? offsetAtoB : offsetBtoA;
    if (mappings.contains(offset)) {
        return mappings[offset];
    }
    return RVA_INVALID;
}

CutterDiffItemDescription CutterDiffItem::functionDescription(const RzAnalysisFunction *func)
{
    CutterDiffItemDescription desc;

    desc["offset"] = static_cast<qulonglong>(func->addr);
    desc["linearSize"] = static_cast<qulonglong>(
            rz_analysis_function_linear_size(const_cast<RzAnalysisFunction *>(func)));
    desc["nargs"] =
            static_cast<qulonglong>(rz_analysis_arg_count(const_cast<RzAnalysisFunction *>(func)));
    desc["nbbs"] = static_cast<qulonglong>(rz_pvector_len(func->bbs));
    desc["nlocals"] = static_cast<qulonglong>(
            rz_analysis_var_local_count(const_cast<RzAnalysisFunction *>(func)));
    desc["calltype"] = QString::fromUtf8(func->cc ? func->cc : "");
    desc["name"] = QString::fromUtf8(func->name ? func->name : "");
    desc["edges"] = static_cast<qulonglong>(rz_analysis_function_count_edges(func, nullptr));
    desc["stackframe"] = static_cast<qulonglong>(func->maxstack);
    desc["disas"] = QStringList {};

    return desc;
}

CutterDiffItemDescription CutterDiffItem::blockDescription(const RzAnalysisBlock *bb)
{
    CutterDiffItemDescription desc;

    desc["offset"] = static_cast<qulonglong>(bb->addr);
    desc["size"] = static_cast<qulonglong>(bb->size);
    desc["fail"] = static_cast<qulonglong>(bb->fail);
    desc["jump"] = static_cast<qulonglong>(bb->jump);

    QList<qulonglong> switchCaseOps;
    const RzAnalysisSwitchOp *switchOp = bb->switch_op;

    if (switchOp) {
        for (const auto &caseOp : CutterRzList<RzAnalysisCaseOp>(switchOp->cases)) {
            if (caseOp->jump == RVA_INVALID) {
                continue;
            }
            switchCaseOps.emplace_back(caseOp->jump);
        }
    }

    desc.insert("casejumps", QVariant::fromValue(switchCaseOps));

    return desc;
}

bool CutterDiffItem::isValidPair(DiffItemType type, const void *a, const void *b)
{
    switch (type) {
    case DiffItemMatched:
        return a != nullptr && b != nullptr;

    case DiffItemRemoved:
        return a != nullptr && b == nullptr;

    case DiffItemAdded:
        return a == nullptr && b != nullptr;
    }
    return false;
}

// AI Generated Starts GPT GO

QJsonObject CutterDiffItem::toJson() const
{
    QJsonObject json;

    json["functionDiff"] = functionDiff;
    json["blockDiff"] = blockDiff;
    json["type"] = static_cast<int>(type);

    json["descA"] = QJsonObject::fromVariantMap(descA);
    json["descB"] = QJsonObject::fromVariantMap(descB);

    json["simtype"] = simtype;
    json["similarity"] = similarity;

    QJsonObject offsetAtoBJson;
    for (auto it = offsetAtoB.constBegin(); it != offsetAtoB.constEnd(); ++it) {
        offsetAtoBJson[QString::number(it.key())] = QString::number(it.value());
    }
    json["offsetAtoB"] = offsetAtoBJson;

    QJsonObject offsetBtoAJson;
    for (auto it = offsetBtoA.constBegin(); it != offsetBtoA.constEnd(); ++it) {
        offsetBtoAJson[QString::number(it.key())] = QString::number(it.value());
    }
    json["offsetBtoA"] = offsetBtoAJson;

    QJsonObject instrDiffsJson;

    for (auto it = instrDiffs.constBegin(); it != instrDiffs.constEnd(); ++it) {
        QJsonArray instructions;

        for (const DiffInstr &instr : it.value()) {
            QJsonObject instruction;

            instruction["a"] = instr.a;
            instruction["b"] = instr.b;
            instruction["type"] = static_cast<int>(instr.type);

            QJsonObject bound;
            bound["pos"] = instr.bound.pos;
            bound["size"] = instr.bound.size;

            instruction["bound"] = bound;
            instructions.append(instruction);
        }

        instrDiffsJson[it.key()] = instructions;
    }

    json["instrDiffs"] = instrDiffsJson;

    // Recursively serialize children.
    QJsonArray blocksJson;

    for (const CutterDiffItem &block : blocks) {
        blocksJson.append(block.toJson());
    }

    json["blocks"] = blocksJson;

    return json;
}

CutterDiffItem CutterDiffItem::fromJson(const QJsonObject &json)
{
    CutterDiffItem item;

    item.functionDiff = json["functionDiff"].toBool();
    item.blockDiff = json["blockDiff"].toBool();

    item.type = static_cast<DiffItemType>(json["type"].toInt());

    item.descA = json["descA"].toObject().toVariantMap();
    item.descB = json["descB"].toObject().toVariantMap();

    item.simtype = json["simtype"].toString();
    item.similarity = json["similarity"].toDouble();

    // offsetAtoB
    const QJsonObject offsetAtoBJson = json["offsetAtoB"].toObject();

    for (auto it = offsetAtoBJson.constBegin(); it != offsetAtoBJson.constEnd(); ++it) {

        item.offsetAtoB.insert(it.key().toULongLong(), it.value().toString().toULongLong());
    }

    // offsetBtoA
    const QJsonObject offsetBtoAJson = json["offsetBtoA"].toObject();

    for (auto it = offsetBtoAJson.constBegin(); it != offsetBtoAJson.constEnd(); ++it) {

        item.offsetBtoA.insert(it.key().toULongLong(), it.value().toString().toULongLong());
    }

    // instrDiffs
    const QJsonObject instrDiffsJson = json["instrDiffs"].toObject();

    for (auto it = instrDiffsJson.constBegin(); it != instrDiffsJson.constEnd(); ++it) {

        QList<DiffInstr> instructions;
        const QJsonArray array = it.value().toArray();

        for (const QJsonValue &value : array) {
            const QJsonObject instructionJson = value.toObject();

            DiffInstr instr;

            instr.a = instructionJson["a"].toString();
            instr.b = instructionJson["b"].toString();

            instr.type = static_cast<DiffInstrType>(instructionJson["type"].toInt());

            const QJsonObject boundJson = instructionJson["bound"].toObject();

            instr.bound.pos = boundJson["pos"].toInt();
            instr.bound.size = boundJson["size"].toInt();

            instructions.append(instr);
        }

        item.instrDiffs.insert(it.key(), instructions);
    }

    // Recursively deserialize children.
    const QJsonArray blocksJson = json["blocks"].toArray();

    for (const QJsonValue &value : blocksJson) {
        item.blocks.append(CutterDiffItem::fromJson(value.toObject()));
    }

    return item;
}

// AI Generated Ends GPT Go

CutterDiff::CutterDiff(QObject *parent)
    : QObject { parent }
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
      ,
      mutex(QMutex::Recursive)
#endif
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
      ,
      analysisMutex(QMutex::Recursive)
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

    if (coreA || coreB) {
        qInfo() << "preInit";
    }

    coreA = rz_core_new();
    coreB = rz_core_new();

    // reassigning console to the main cutter core till the context based console is ready
    {
        RzCoreLocked core(Core());
        core->cons->line->user = core;
        core->cons->line->cb_fkey = core->cons->cb_fkey;
        core->cons->user_fgets_user = core;
        rz_core_bind_cons(core);
    }

    if (!(coreA || coreB)) {
        goto fail;
    }

    rz_core_loadlibs(coreA, RZ_CORE_LOADLIBS_ALL);
    rz_core_loadlibs(coreB, RZ_CORE_LOADLIBS_ALL);

    coreA->print->scr_prompt = false;
    coreB->print->scr_prompt = false;

    return true;
fail:
    qWarning() << "Core initialization has failed undefined behaviour expected.";
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
    filePathA = fileA;
    filePathB = fileB;
    fileNameA = QFileInfo(filePathA).fileName();
    fileNameB = QFileInfo(filePathB).fileName();
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
    return nullptr;
}

RzAnalysisMatchResult *CutterDiff::matchFunctionBlocks(RVA addrA, RVA addrB,
                                                       RzAnalysisMatchThreadInfoCb callback,
                                                       void *user)
{
    RzAnalysisFunction *funcA = rz_analysis_get_function_at(coreA->analysis, addrA);
    RzAnalysisFunction *funcB = rz_analysis_get_function_at(coreB->analysis, addrB);
    RzAnalysisMatchResult *results = nullptr;
    RzAnalysisMatchOpt opts;
    opts.analysis_a = coreA->analysis;
    opts.analysis_b = coreB->analysis;
    opts.callback = callback;
    opts.user = user;
    if (!funcA || !funcB) {
        return results;
    }
    results = rz_analysis_match_basic_blocks(funcA, funcB, &opts);
    if (!results) {
        qWarning() << tr("failed to perform the function matching operation or job was cancelled.");
    }
    return results;
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

QString CutterDiff::ansiEscapeToHtml(const QString &text)
{
    int len;
    QString r = text;
    r.replace("\t", "        ");
    char *html = rz_cons_html_filter(
            r.toUtf8().constData(),
            &len); // doesn't support background colors work has to be done in the rizin side
    if (!html) {
        return {};
    }
    r = QString::fromUtf8(html, len);
    rz_mem_free(html);
    return r;
}

QString CutterDiff::disassembleFunction(RVA addr, bool orig)
{
    LOCK();
    RzCore *core = orig ? coreA : coreB;
    RzAnalysisFunction *function = rz_analysis_get_fcn_in(
            core->analysis, addr, RZ_ANALYSIS_FCN_TYPE_FCN | RZ_ANALYSIS_FCN_TYPE_SYM);
    if (!function) {
        qWarning() << QString("Could not load function at %1").arg(QString::number(addr, 16));
        return {};
    }
    auto vec = fromOwned(
            rz_pvector_new(reinterpret_cast<RzPVectorFree>(rz_analysis_disasm_text_free)));
    if (!vec) {
        return {};
    }
    const uint64_t start = function->addr;
    const uint64_t end = rz_analysis_function_max_addr(function);
    if (start > end) {
        qWarning() << "Start address is greater than end address of the function";
        return {};
    }
    const uint64_t size = end - start;
    QByteArray array;
    array.resize(size);
    rz_io_read_at_mapped(core->io, start, reinterpret_cast<ut8 *>(array.data()), size);
    RzCoreDisasmOptions disasmOptions = { .cbytes = 1, .function = function, .vec = vec.get() };
    TempDiffConfig config(this, orig);
    config.setConfigi("scr.utf8", 0);
    config.setConfigi("asm.offset", 0);
    config.setConfigi("asm.lines", 0);
    config.setConfigi("asm.cmt.right", 0);
    config.setConfigi("asm.lines.fcn", 0);
    config.setConfigi("asm.bytes", 0);
    config.setConfigi("asm.comments", 0);
    config.setConfigi("scr.color", COLOR_MODE_DISABLED);
    rz_core_print_disasm(core, start, reinterpret_cast<ut8 *>(array.data()), size, size, nullptr,
                         &disasmOptions);
    QString r;
    for (const auto &t : CutterPVector<RzAnalysisDisasmText>(vec.get())) {
        const QString text = t->text;
        r.append(text);
        r.append("\n");
    }
    return r;
}

QString CutterDiff::disassembleBasicBlock(RVA addr, bool orig)
{
    LOCK();
    RzCore *core = orig ? coreA : coreB;
    RzAnalysisBlock *bbi = rz_analysis_get_block_at(core->analysis, addr);
    if (!bbi) {
        qWarning() << QString("Could not load basic block at %1").arg(QString::number(addr, 16));
        return {};
    }
    auto vec = fromOwned(
            rz_pvector_new(reinterpret_cast<RzPVectorFree>(rz_analysis_disasm_text_free)));
    if (!vec) {
        return {};
    }
    const uint64_t start = bbi->addr;
    const uint64_t end = start + bbi->size;
    if (start > end) {
        qWarning() << "Start address is greater than end address of the function";
        return {};
    }
    const uint64_t size = bbi->size;
    QByteArray array;
    array.resize(size);
    rz_io_read_at_mapped(core->io, start, reinterpret_cast<ut8 *>(array.data()), size);
    RzCoreDisasmOptions disasmOptions = { .cbytes = 1, .vec = vec.get() };
    TempDiffConfig config(this, orig);
    config.setConfigi("scr.utf8", 0);
    config.setConfigi("asm.offset", 0);
    config.setConfigi("asm.lines", 0);
    config.setConfigi("asm.cmt.right", 0);
    config.setConfigi("asm.lines.fcn", 0);
    config.setConfigi("asm.bytes", 0);
    config.setConfigi("asm.comments", 0);
    config.setConfigi("scr.color", COLOR_MODE_DISABLED);
    rz_core_print_disasm(core, start, reinterpret_cast<ut8 *>(array.data()), size, size, nullptr,
                         &disasmOptions);
    QString r;
    for (const auto &t : CutterPVector<RzAnalysisDisasmText>(vec.get())) {
        const QString text = t->text;
        r.append(text);
        r.append("\n");
    }
    return r;
}

RzDiff *CutterDiff::diffFunctionDissas(RVA addrA, RVA addrB)
{
    LOCK();
    const QString disasA = disassembleFunction(addrA, true);
    const QString disasB = disassembleFunction(addrB, false);
    return lineDiff(disasA, disasB);
}

RzDiff *CutterDiff::diffBlockDisas(RVA addrA, RVA addrB)
{
    LOCK();
    const QString disasA = disassembleBasicBlock(addrA, true);
    const QString disasB = disassembleBasicBlock(addrB, false);
    return lineDiff(disasA, disasB);
}

RzDiff *CutterDiff::lineDiff(const char *lines1, const char *lines2)
{
    RzDiff *diff = rz_diff_lines_new(lines1, lines2, nullptr);
    return diff;
}

RzDiff *CutterDiff::lineDiff(const QString &lines1, const QString &lines2)
{
    return lineDiff(lines1.toUtf8().constData(), lines2.toUtf8().constData());
}

RzList *CutterDiff::lineDiffOpsGrouped(RzDiff *diff) const
{
    auto *groups = rz_diff_opcodes_grouped_new(diff, 3);
    return groups;
}

Bound CutterDiff::getLineDiffBounds(const QString &line1, const QString &line2) const
{
    if (line1.size() != line2.size()) {
        return { 0, 0 };
    }

    int first = 0;
    while (first < line1.size() && line1[first] == line2[first]) {
        ++first;
    }

    int last = line1.size() - 1;
    while (last >= first && line1[last] == line2[last]) {
        --last;
    }

    return { first, last - first + 1 };
}

BinDiffMatchDescription CutterDiff::getCurrentMatchDescription() const
{
    if (getCurrentDiffItem().getType() == DiffItemMatched) {
        return getCurrentDiffItem().toBinDiffMatchDescription();
    }
    return {};
}

bool CutterDiff::saveDiffItemsToJson(const QString &filePath) const
{
    QJsonArray diffItemsJson;

    for (const CutterDiffItem &item : diffItemList) {
        diffItemsJson.append(item.toJson());
    }

    QJsonObject root;
    root["diffItems"] = diffItemsJson;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    const QJsonDocument document(root);
    file.write(document.toJson(QJsonDocument::Indented));

    return true;
}

bool CutterDiff::loadDiffItemsFromJson(const QString &filePath)
{
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());

    if (!document.isObject()) {
        return false;
    }

    const QJsonArray diffItemsJson = document.object()["diffItems"].toArray();

    diffItemList.clear();

    for (const QJsonValue &value : diffItemsJson) {
        if (!value.isObject()) {
            continue;
        }

        diffItemList.append(CutterDiffItem::fromJson(value.toObject()));
    }

    return true;
}

QList<DiffInstr> CutterDiff::rzDiffOpToCutterInstrs(RzDiff *diff,
                                                    RzList * /*<RzList<RzDiffOp*>>**/ list) const
{
    QList<DiffInstr> result;
    char *stringUtf;
    const RzListIter *it = nullptr;
    const RzList *group = nullptr;
    CutterRzListForeach (list, it, RzList /*<RzDiffOp *>*/, group) {
        for (RzDiffOp *op : CutterRzList<RzDiffOp>(group)) {
            DiffInstr instr;
            switch (op->type) {
            case RZ_DIFF_OP_EQUAL: {
                stringUtf = rz_diff_op_stringify(diff, op, true);
                const QString opString = QString::fromUtf8(stringUtf);
                instr.a = opString;
                instr.type = DiffInstrEqual;
                break;
            }
            case RZ_DIFF_OP_DELETE: {
                stringUtf = rz_diff_op_stringify(diff, op, true);
                const QString opString = QString::fromUtf8(stringUtf);
                instr.a = opString;
                instr.type = DiffInstrDeleted;
                break;
            }
            case RZ_DIFF_OP_INSERT: {
                stringUtf = rz_diff_op_stringify(diff, op, false);
                const QString opString = QString::fromUtf8(stringUtf);
                instr.b = opString;
                instr.type = DiffInstrInserted;
                break;
            }
            case RZ_DIFF_OP_REPLACE: {
                const QString actual = QString::fromUtf8(rz_diff_op_stringify(diff, op, true));
                const QString replaced = QString::fromUtf8(rz_diff_op_stringify(diff, op, false));
                const auto bound = getLineDiffBounds(actual, replaced);
                instr.a = actual;
                instr.b = replaced;
                instr.type = DiffInstrReplaced;
                instr.bound = bound;
                break;
            }
            default:
                break;
            }
            result.emplace_back(instr);
        }
    }
    return result;
}