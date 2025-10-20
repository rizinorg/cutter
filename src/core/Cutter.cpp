#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QDir>
#include <QCoreApplication>
#include <QVector>
#include <QStringList>
#include <QStandardPaths>

#include <cassert>
#include <memory>

#include "CutterDescriptions.h"
#include "common/TempConfig.h"
#include "common/BasicInstructionHighlighter.h"
#include "common/Configuration.h"
#include "common/AsyncTask.h"
#include "common/RizinTask.h"
#include "dialogs/MarkDialog.h"
#include "dialogs/RizinTaskDialog.h"
#include "common/Json.h"
#include "core/Cutter.h"
#include "Decompiler.h"

#include <rz_asm.h>
#include <rz_cmd.h>
#include <rz_socket.h>
#include <sdb.h>

static CutterCore *uniqueInstance;

#define RZ_JSON_KEY(name) static const QString name = QStringLiteral(#name)

namespace RJsonKey {
RZ_JSON_KEY(addr);
RZ_JSON_KEY(address);
RZ_JSON_KEY(addrs);
RZ_JSON_KEY(addr_end);
RZ_JSON_KEY(arrow);
RZ_JSON_KEY(baddr);
RZ_JSON_KEY(bind);
RZ_JSON_KEY(blocks);
RZ_JSON_KEY(blocksize);
RZ_JSON_KEY(bytes);
RZ_JSON_KEY(calltype);
RZ_JSON_KEY(cc);
RZ_JSON_KEY(classname);
RZ_JSON_KEY(code);
RZ_JSON_KEY(comment);
RZ_JSON_KEY(comments);
RZ_JSON_KEY(cost);
RZ_JSON_KEY(data);
RZ_JSON_KEY(description);
RZ_JSON_KEY(ebbs);
RZ_JSON_KEY(edges);
RZ_JSON_KEY(enabled);
RZ_JSON_KEY(entropy);
RZ_JSON_KEY(fcn_addr);
RZ_JSON_KEY(fcn_name);
RZ_JSON_KEY(fields);
RZ_JSON_KEY(file);
RZ_JSON_KEY(flag);
RZ_JSON_KEY(flags);
RZ_JSON_KEY(flagname);
RZ_JSON_KEY(format);
RZ_JSON_KEY(from);
RZ_JSON_KEY(functions);
RZ_JSON_KEY(graph);
RZ_JSON_KEY(haddr);
RZ_JSON_KEY(hw);
RZ_JSON_KEY(in_functions);
RZ_JSON_KEY(index);
RZ_JSON_KEY(jump);
RZ_JSON_KEY(laddr);
RZ_JSON_KEY(lang);
RZ_JSON_KEY(len);
RZ_JSON_KEY(length);
RZ_JSON_KEY(license);
RZ_JSON_KEY(methods);
RZ_JSON_KEY(name);
RZ_JSON_KEY(realname);
RZ_JSON_KEY(nargs);
RZ_JSON_KEY(nbbs);
RZ_JSON_KEY(nlocals);
RZ_JSON_KEY(offset);
RZ_JSON_KEY(opcode);
RZ_JSON_KEY(opcodes);
RZ_JSON_KEY(ordinal);
RZ_JSON_KEY(libname);
RZ_JSON_KEY(outdegree);
RZ_JSON_KEY(paddr);
RZ_JSON_KEY(path);
RZ_JSON_KEY(perm);
RZ_JSON_KEY(pid);
RZ_JSON_KEY(plt);
RZ_JSON_KEY(prot);
RZ_JSON_KEY(ref);
RZ_JSON_KEY(refs);
RZ_JSON_KEY(reg);
RZ_JSON_KEY(rwx);
RZ_JSON_KEY(section);
RZ_JSON_KEY(sections);
RZ_JSON_KEY(size);
RZ_JSON_KEY(stackframe);
RZ_JSON_KEY(status);
RZ_JSON_KEY(string);
RZ_JSON_KEY(strings);
RZ_JSON_KEY(symbols);
RZ_JSON_KEY(text);
RZ_JSON_KEY(to);
RZ_JSON_KEY(trace);
RZ_JSON_KEY(type);
RZ_JSON_KEY(uid);
RZ_JSON_KEY(vaddr);
RZ_JSON_KEY(value);
RZ_JSON_KEY(vsize);
}

#undef RZ_JSON_KEY

static void updateOwnedCharPtr(char *&variable, const QString &newValue)
{
    auto data = newValue.toUtf8();
    RZ_FREE(variable)
    variable = strdup(data.data());
}

static bool reg_sync(RzCore *core, RzRegisterType type, bool write)
{
    if (rz_core_is_debug(core)) {
        return rz_debug_reg_sync(core->dbg, type, write);
    }
    return true;
}

RzCoreLocked::RzCoreLocked(CutterCore *core) : core(core)
{
    core->coreMutex.lock();
    assert(core->coreLockDepth >= 0);
    core->coreLockDepth++;
    if (core->coreLockDepth == 1) {
        assert(core->coreBed);
        rz_cons_sleep_end(core->coreBed);
        core->coreBed = nullptr;
    }
}

RzCoreLocked::~RzCoreLocked()
{
    assert(core->coreLockDepth > 0);
    core->coreLockDepth--;
    if (core->coreLockDepth == 0) {
        core->coreBed = rz_cons_sleep_begin();
    }
    core->coreMutex.unlock();
}

RzCoreLocked::operator RzCore *() &
{
    return core->core_;
}

RzCore *RzCoreLocked::operator->() &
{
    return core->core_;
}

#define CORE_LOCK() RzCoreLocked core(this)

static void cutterREventCallback(RzEvent *, int type, void *user, void *data)
{
    auto core = reinterpret_cast<CutterCore *>(user);
    core->handleREvent(type, data);
}

CutterCore::CutterCore(QObject *parent)
    : QObject(parent)
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
      ,
      coreMutex(QMutex::Recursive)
#endif
{
    if (uniqueInstance) {
        throw std::logic_error("Only one instance of CutterCore must exist");
    }
    uniqueInstance = this;
}

CutterCore *CutterCore::instance()
{
    return uniqueInstance;
}

void CutterCore::initialize(bool loadPlugins)
{
    rz_cons_new(); // initialize console
    core_ = rz_core_new();

#if defined(MACOS_RZ_BUNDLED)
    auto app_path = QDir(QCoreApplication::applicationDirPath());
    app_path.cdUp();
    app_path.cd("Resources");
    qInfo() << "Setting Rizin prefix =" << app_path.absolutePath()
            << " for macOS Application Bundle.";
    rz_path_set_prefix(core_->sys_path, app_path.absolutePath().toUtf8().constData());
#endif

    char **env = rz_sys_get_environ();
    core_->io->envprofile = rz_run_get_environ_profile(env);
    rz_core_task_sync_begin(&core_->tasks);
    coreBed = rz_cons_sleep_begin();
    CORE_LOCK();

    rz_event_hook(core_->analysis->ev, RZ_EVENT_ALL, cutterREventCallback, this);

    if (loadPlugins) {
        setConfig("cfg.plugins", true);
        rz_core_loadlibs(this->core_, RZ_CORE_LOADLIBS_ALL);
    } else {
        setConfig("cfg.plugins", false);
    }
    // IMPLICIT rz_bin_iobind (core_->bin, core_->io);

    // Otherwise Rizin may ask the user for input and Cutter would freeze
    setConfig("scr.interactive", false);

    // Temporary workaround for https://github.com/rizinorg/rizin/issues/2741
    // Otherwise sometimes disassembly is truncated.
    // The blocksize here is a rather arbitrary value larger than the default 0x100.
    rz_core_block_size(core, 0x400);

    // Initialize graph node highlighter
    bbHighlighter = new BasicBlockHighlighter();

    // Initialize Async tasks manager
    asyncTaskManager = new AsyncTaskManager(this);
}

CutterCore::~CutterCore()
{
    delete bbHighlighter;
    rz_cons_sleep_end(coreBed);
    rz_core_task_sync_end(&core_->tasks);
    rz_core_free(this->core_);
    rz_cons_free();
    assert(uniqueInstance == this);
    uniqueInstance = nullptr;
}

RzCoreLocked CutterCore::lock()
{
    return RzCoreLocked(this);
}

RzCoreLocked CutterCore::core()
{
    return lock();
}

QDir CutterCore::getCutterRCDefaultDirectory() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
}

QVector<QString> CutterCore::getCutterRCFilePaths() const
{
    QVector<QString> result;
    result.push_back(QFileInfo(QDir::home(), ".cutterrc").absoluteFilePath());
    QStringList locations = QStandardPaths::standardLocations(QStandardPaths::AppConfigLocation);
    for (auto &location : locations) {
        result.push_back(QFileInfo(QDir(location), ".cutterrc").absoluteFilePath());
    }
    result.push_back(QFileInfo(getCutterRCDefaultDirectory(), "rc")
                             .absoluteFilePath()); // File in config editor is from this path
    return result;
}

void CutterCore::loadCutterRC()
{
    CORE_LOCK();
    const auto result = getCutterRCFilePaths();
    for (auto &cutterRCFilePath : result) {
        auto cutterRCFileInfo = QFileInfo(cutterRCFilePath);
        if (!cutterRCFileInfo.exists() || !cutterRCFileInfo.isFile()) {
            continue;
        }
        qInfo() << tr("Loading initialization file from ") << cutterRCFilePath;
        rz_core_cmd_file(core, cutterRCFilePath.toUtf8().constData());
        rz_cons_flush();
    }
}

void CutterCore::loadDefaultCutterRC()
{
    CORE_LOCK();
    auto cutterRCFilePath = QFileInfo(getCutterRCDefaultDirectory(), "rc").absoluteFilePath();
    const auto cutterRCFileInfo = QFileInfo(cutterRCFilePath);
    if (!cutterRCFileInfo.exists() || !cutterRCFileInfo.isFile()) {
        return;
    }
    qInfo() << tr("Loading initialization file from ") << cutterRCFilePath;
    rz_core_cmd_file(core, cutterRCFilePath.toUtf8().constData());
    rz_cons_flush();
}

QList<QString> CutterCore::sdbList(QString path)
{
    CORE_LOCK();
    QList<QString> list = QList<QString>();
    Sdb *root = sdb_ns_path(core->sdb, path.toUtf8().constData(), 0);
    if (root && root->ns) {
        for (const auto &nsi : CutterRzList<SdbNs>(root->ns)) {
            list << nsi->name;
        }
    }
    return list;
}

using PVectorPtr = std::unique_ptr<RzPVector, decltype(&rz_pvector_free)>;
static PVectorPtr makePVectorPtr(RzPVector *vec)
{
    return { vec, rz_pvector_free };
}

static bool foreach_keys_cb(void *user, const SdbKv *kv)
{
    auto list = reinterpret_cast<QList<QString> *>(user);
    *list << kv->base.key;
    return true;
}

QList<QString> CutterCore::sdbListKeys(QString path)
{
    CORE_LOCK();
    QList<QString> list = QList<QString>();
    Sdb *root = sdb_ns_path(core->sdb, path.toUtf8().constData(), 0);
    if (root) {
        sdb_foreach(root, foreach_keys_cb, &list);
    }
    return list;
}

QString CutterCore::sdbGet(QString path, QString key)
{
    CORE_LOCK();
    Sdb *db = sdb_ns_path(core->sdb, path.toUtf8().constData(), 0);
    if (db) {
        const char *val = sdb_const_get(db, key.toUtf8().constData());
        if (val && *val)
            return val;
    }
    return QString();
}

bool CutterCore::sdbSet(QString path, QString key, QString val)
{
    CORE_LOCK();
    Sdb *db = sdb_ns_path(core->sdb, path.toUtf8().constData(), 1);
    if (!db)
        return false;
    return sdb_set(db, key.toUtf8().constData(), val.toUtf8().constData());
}

QString CutterCore::sanitizeStringForCommand(QString s)
{
    static const QRegularExpression regexp(";|@");
    return s.replace(regexp, QStringLiteral("_"));
}

QString CutterCore::cmd(const char *str)
{
    CORE_LOCK();

    RVA offset = core->offset;
    char *res = rz_core_cmd_str(core, str);
    QString o = fromOwnedCharPtr(res);

    if (offset != core->offset) {
        updateSeek();
    }
    return o;
}

QString CutterCore::getFunctionExecOut(const std::function<bool(RzCore *)> &fcn, const RVA addr)
{
    CORE_LOCK();

    RVA offset = core->offset;
    seekSilent(addr);
    QString o = {};
    rz_cons_push();
    bool is_pipe = core->is_pipe;
    core->is_pipe = true;

    if (!fcn(core)) {
        core->is_pipe = is_pipe;
        rz_cons_pop();
        goto clean_return;
    }

    core->is_pipe = is_pipe;
    rz_cons_filter();
    o = rz_cons_get_buffer();

    rz_cons_pop();
    rz_cons_echo(NULL);

clean_return:
    if (offset != core->offset) {
        seekSilent(offset);
    }
    return o;
}

bool CutterCore::isRedirectableDebugee()
{
    if (!currentlyDebugging || currentlyAttachedToPID != -1) {
        return false;
    }

    // We are only able to redirect locally debugged unix processes
    RzCoreLocked core(Core());
    RzList *descs = rz_id_storage_list(core->io->files);
    RzListIter *it;
    RzIODesc *desc;
    CutterRzListForeach (descs, it, RzIODesc, desc) {
        QString URI = QString(desc->uri);
        if (URI.contains("ptrace") || URI.contains("mach")) {
            return true;
        }
    }
    return false;
}

bool CutterCore::isDebugTaskInProgress()
{
    if (!debugTask.isNull()) {
        return true;
    }

    return false;
}

bool CutterCore::asyncTask(std::function<void *(RzCore *)> fcn, QSharedPointer<RizinTask> &task)
{
    if (!task.isNull()) {
        return false;
    }

    CORE_LOCK();
    RVA offset = core->offset;
    task = QSharedPointer<RizinTask>(new RizinFunctionTask(std::move(fcn), true));
    connect(task.data(), &RizinTask::finished, task.data(), [this, offset, task]() {
        CORE_LOCK();

        if (offset != core->offset) {
            updateSeek();
        }
    });

    return true;
}

void CutterCore::functionTask(std::function<void *(RzCore *)> fcn)
{
    auto task = std::unique_ptr<RizinTask>(new RizinFunctionTask(std::move(fcn), true));
    task->startTask();
    task->joinTask();
}

QString CutterCore::cmdRawAt(const char *cmd, RVA address)
{
    QString res;
    RVA oldOffset = getOffset();
    seekSilent(address);

    res = cmdRaw(cmd);

    seekSilent(oldOffset);
    return res;
}

QString CutterCore::cmdRaw(const char *cmd)
{
    QString res;
    CORE_LOCK();
    return rz_core_cmd_str(core, cmd);
}

CutterJson CutterCore::cmdj(const char *str)
{
    char *res;
    {
        CORE_LOCK();
        res = rz_core_cmd_str(core, str);
    }

    return parseJson("cmdj", res, str);
}

QString CutterCore::cmdTask(const QString &str)
{
    RizinCmdTask task(str);
    task.startTask();
    task.joinTask();
    return task.getResult();
}

CutterJson CutterCore::parseJson(const char *name, char *res, const char *cmd)
{
    if (RZ_STR_ISEMPTY(res)) {
        return CutterJson();
    }

    RzJson *doc = rz_json_parse(res);

    if (!doc) {
        if (cmd) {
            RZ_LOG_ERROR("%s: Failed to parse JSON for command \"%s\"\n%s\n", name, cmd, res);
        } else {
            RZ_LOG_ERROR("%s: Failed to parse JSON %s\n", name, res);
        }
        RZ_LOG_ERROR("%s: %s\n", name, res);
    }

    return CutterJson(doc, QSharedPointer<CutterJsonOwner>::create(doc, res));
}

QStringList CutterCore::autocomplete(const QString &cmd, RzLinePromptType promptType)
{
    RzLineBuffer buf;
    int c = snprintf(buf.data, sizeof(buf.data), "%s", cmd.toUtf8().constData());
    if (c < 0) {
        return {};
    }
    buf.index = buf.length = std::min((int)(sizeof(buf.data) - 1), c);

    CORE_LOCK();
    RzLineNSCompletionResult *compr = rz_core_autocomplete_rzshell(core, &buf, promptType);

    QStringList r;
    auto optslen = rz_pvector_len(&compr->options);
    r.reserve(optslen);
    for (size_t i = 0; i < optslen; i++) {
        r.push_back(QString::fromUtf8(
                reinterpret_cast<const char *>(rz_pvector_at(&compr->options, i))));
    }
    rz_line_ns_completion_result_free(compr);

    return r;
}

/**
 * @brief CutterCore::loadFile
 * Load initial file.
 * @param path File path
 * @param baddr Base (RzBin) address
 * @param mapaddr Map address
 * @param perms
 * @param va
 * @param loadbin Load RzBin information
 * @param forceBinPlugin
 * @return
 */
bool CutterCore::loadFile(QString path, ut64 baddr, ut64 mapaddr, int perms, int va, bool loadbin,
                          const QString &forceBinPlugin)
{
    CORE_LOCK();
    RzCoreFile *f;
    rz_config_set_i(core->config, "io.va", va);

    f = rz_core_file_open(core, path.toUtf8().constData(), perms, mapaddr);
    if (!f) {
        eprintf("rz_core_file_open failed\n");
        return false;
    }

    if (!forceBinPlugin.isNull()) {
        rz_bin_force_plugin(rz_core_get_bin(core), forceBinPlugin.toUtf8().constData());
    }

    if (loadbin && va) {
        if (!rz_core_bin_load(core, path.toUtf8().constData(), baddr)) {
            eprintf("CANNOT GET RBIN INFO\n");
        }

#if HAVE_MULTIPLE_RBIN_FILES_INSIDE_SELECT_WHICH_ONE
        if (!rz_core_file_open(core, path.toUtf8(), RZ_IO_READ | (rw ? RZ_IO_WRITE : 0, mapaddr))) {
            eprintf("Cannot open file\n");
        } else {
            // load RzBin information
            // XXX only for sub-bins
            rz_core_bin_load(core, path.toUtf8(), baddr);
            rz_bin_select_idx(core->bin, NULL, idx);
        }
#endif
    }

    auto iod = core->io ? core->io->desc : NULL;
    auto debug =
            core->file && iod && (core->file->fd == iod->fd) && iod->plugin && iod->plugin->isdbg;

    if (!debug && rz_flag_get(core->flags, "entry0")) {
        ut64 addr = rz_num_math(core->num, "entry0");
        rz_core_seek_and_save(core, addr, true);
    }

    if (perms & RZ_PERM_W) {
        RzPVector *maps = rz_io_maps(core->io);
        for (auto map : CutterPVector<RzIOMap>(maps)) {
            map->perm |= RZ_PERM_W;
        }
    }

    rz_cons_flush();
    fflush(stdout);
    return true;
}

bool CutterCore::tryFile(QString path, bool rw)
{
    if (path.isEmpty()) {
        // opening no file is always possible
        return true;
    }
    CORE_LOCK();
    RzCoreFile *cf;
    int flags = RZ_PERM_R;
    if (rw)
        flags = RZ_PERM_RW;
    cf = rz_core_file_open(core, path.toUtf8().constData(), flags, 0LL);
    if (!cf) {
        return false;
    }

    rz_core_file_close(cf);

    return true;
}

/**
 * @brief Maps a file using Rizin API
 * @param path Path to file
 * @param mapaddr Map Address
 * @return bool
 */
bool CutterCore::mapFile(QString path, RVA mapaddr)
{
    CORE_LOCK();
    RVA addr = mapaddr != RVA_INVALID ? mapaddr : 0;
    ut64 baddr = rz_bin_get_baddr(core->bin);
    if (rz_core_file_open(core, path.toUtf8().constData(), RZ_PERM_RX, addr)) {
        rz_core_bin_load(core, path.toUtf8().constData(), baddr);
    } else {
        return false;
    }
    return true;
}

void CutterCore::renameFunction(const RVA offset, const QString &newName)
{
    CORE_LOCK();
    rz_core_analysis_function_rename(core, offset, newName.toStdString().c_str());
    emit functionRenamed(offset, newName);
}

void CutterCore::delFunction(RVA addr)
{
    CORE_LOCK();
    rz_core_analysis_undefine(core, addr);
    emit functionsChanged();
}

void CutterCore::renameFlag(QString old_name, QString new_name)
{
    CORE_LOCK();
    RzFlagItem *flag = rz_flag_get(core->flags, old_name.toStdString().c_str());
    if (!flag)
        return;
    rz_flag_rename(core->flags, flag, new_name.toStdString().c_str());
    emit flagsChanged();
}

void CutterCore::renameFunctionVariable(QString newName, QString oldName, RVA functionAddress)
{
    CORE_LOCK();
    RzAnalysisFunction *function = rz_analysis_get_function_at(core->analysis, functionAddress);
    RzAnalysisVar *variable =
            rz_analysis_function_get_var_byname(function, oldName.toUtf8().constData());
    if (variable) {
        rz_analysis_var_rename(variable, newName.toUtf8().constData(), true);
    }
    emit refreshCodeViews();
}

void CutterCore::delFlag(RVA addr)
{
    CORE_LOCK();
    rz_flag_unset_off(core->flags, addr);
    emit flagsChanged();
}

void CutterCore::delFlag(const QString &name)
{
    CORE_LOCK();
    rz_flag_unset_name(core->flags, name.toStdString().c_str());
    emit flagsChanged();
}

CutterRzIter<RzAnalysisBytes> CutterCore::getRzAnalysisBytesSingle(RVA addr)
{
    CORE_LOCK();
    ut8 buf[128];
    rz_io_read_at(core->io, addr, buf, sizeof(buf));

    // Warning! only safe to use with stack buffer, due to instruction count being 1
    auto result =
            CutterRzIter<RzAnalysisBytes>(rz_core_analysis_bytes(core, addr, buf, sizeof(buf), 1));
    return result;
}

QString CutterCore::getInstructionBytes(RVA addr)
{
    auto ab = getRzAnalysisBytesSingle(addr);
    return ab ? ab->bytes : "";
}

QString CutterCore::getInstructionOpcode(RVA addr)
{
    auto ab = getRzAnalysisBytesSingle(addr);
    return ab ? ab->opcode : "";
}

void CutterCore::editInstruction(RVA addr, const QString &inst, bool fillWithNops)
{
    CORE_LOCK();
    if (fillWithNops) {
        rz_core_write_assembly_fill(core, addr, inst.trimmed().toStdString().c_str());
    } else {
        rz_core_write_assembly(core, addr, inst.trimmed().toStdString().c_str());
    }
    emit instructionChanged(addr);
}

void CutterCore::nopInstruction(RVA addr)
{
    CORE_LOCK();
    {
        auto seek = seekTemp(addr);
        rz_core_hack(core, "nop");
    }
    emit instructionChanged(addr);
}

void CutterCore::jmpReverse(RVA addr)
{
    CORE_LOCK();
    {
        auto seek = seekTemp(addr);
        rz_core_hack(core, "recj");
    }
    emit instructionChanged(addr);
}

void CutterCore::editBytes(RVA addr, const QString &bytes)
{
    CORE_LOCK();
    rz_core_write_hexpair(core, addr, bytes.toUtf8().constData());
    emit instructionChanged(addr);
}

void CutterCore::editBytesEndian(RVA addr, const QString &bytes)
{
    CORE_LOCK();
    ut64 value = rz_num_math(core->num, bytes.toUtf8().constData());
    if (core->num->nc.errors) {
        return;
    }
    rz_core_write_value_at(core, addr, value, 0);
    emit stackChanged();
}

void CutterCore::setToCode(RVA addr)
{
    CORE_LOCK();
    rz_meta_del(core->analysis, RZ_META_TYPE_STRING, core->offset, 1);
    rz_meta_del(core->analysis, RZ_META_TYPE_DATA, core->offset, 1);
    emit instructionChanged(addr);
}

void CutterCore::setAsString(RVA addr, int size, StringTypeFormats type)
{
    if (RVA_INVALID == addr) {
        return;
    }

    RzStrEnc encoding;
    switch (type) {
    case StringTypeFormats::None: {
        encoding = RZ_STRING_ENC_GUESS;
        break;
    }
    case StringTypeFormats::ASCII_LATIN1: {
        encoding = RZ_STRING_ENC_8BIT;
        break;
    }
    case StringTypeFormats::UTF8: {
        encoding = RZ_STRING_ENC_UTF8;
        break;
    }
    default:
        return;
    }

    CORE_LOCK();
    seekAndShow(addr);
    rz_core_meta_string_add(core, addr, size, encoding, nullptr);
    emit instructionChanged(addr);
}

void CutterCore::removeString(RVA addr)
{
    CORE_LOCK();
    rz_meta_del(core->analysis, RZ_META_TYPE_STRING, addr, 1);
    emit instructionChanged(addr);
}

QString CutterCore::getString(RVA addr)
{
    CORE_LOCK();
    return getString(addr, core->blocksize,
                     rz_str_guess_encoding_from_buffer(core->block, core->blocksize));
}

QString CutterCore::getString(RVA addr, uint64_t len, RzStrEnc encoding, bool escape_nl)
{
    CORE_LOCK();
    RzStrStringifyOpt opt = {};
    opt.buffer = core->block;
    opt.length = len;
    opt.encoding = encoding;
    opt.escape_nl = escape_nl;
    auto seek = seekTemp(addr);
    return fromOwnedCharPtr(rz_str_stringify_raw_buffer(&opt, NULL));
}

QString CutterCore::getMetaString(RVA addr)
{
    CORE_LOCK();
    return rz_meta_get_string(core->analysis, RZ_META_TYPE_STRING, addr);
}

void CutterCore::setToData(RVA addr, int size, int repeat)
{
    if (size <= 0 || repeat <= 0) {
        return;
    }

    CORE_LOCK();
    RVA address = addr;
    for (int i = 0; i < repeat; ++i, address += size) {
        rz_meta_set(core->analysis, RZ_META_TYPE_DATA, address, size, nullptr);
    }
    emit instructionChanged(addr);
}

int CutterCore::sizeofDataMeta(RVA addr)
{
    ut64 size;
    CORE_LOCK();
    rz_meta_get_at(core->analysis, addr, RZ_META_TYPE_DATA, &size);
    return (int)size;
}

void CutterCore::setComment(RVA addr, const QString &cmt)
{
    CORE_LOCK();
    rz_meta_set_string(core->analysis, RZ_META_TYPE_COMMENT, addr, cmt.toStdString().c_str());
    emit commentsChanged(addr);
}

void CutterCore::delComment(RVA addr)
{
    CORE_LOCK();
    rz_meta_del(core->analysis, RZ_META_TYPE_COMMENT, addr, 1);
    emit commentsChanged(addr);
}

/**
 * @brief Gets the comment present at a specific address
 * @param addr The address to be checked
 * @return String containing comment
 */
QString CutterCore::getCommentAt(RVA addr)
{
    CORE_LOCK();
    return rz_meta_get_string(core->analysis, RZ_META_TYPE_COMMENT, addr);
}

void CutterCore::setImmediateBase(const QString &rzBaseName, RVA offset)
{
    if (offset == RVA_INVALID) {
        offset = getOffset();
    }
    CORE_LOCK();
    int base = (int)rz_num_base_of_string(core->num, rzBaseName.toUtf8().constData());
    rz_analysis_hint_set_immbase(core->analysis, offset, base);
    emit instructionChanged(offset);
}

void CutterCore::setCurrentBits(int bits, RVA offset)
{
    if (offset == RVA_INVALID) {
        offset = getOffset();
    }

    CORE_LOCK();
    rz_analysis_hint_set_bits(core->analysis, offset, bits);
    emit instructionChanged(offset);
}

void CutterCore::applyStructureOffset(const QString &structureOffset, RVA offset)
{
    if (offset == RVA_INVALID) {
        offset = getOffset();
    }

    {
        CORE_LOCK();
        auto seek = seekTemp(offset);
        rz_core_analysis_hint_set_offset(core, structureOffset.toUtf8().constData());
    }
    emit instructionChanged(offset);
}

void CutterCore::seekSilent(ut64 offset)
{
    CORE_LOCK();
    if (offset == RVA_INVALID) {
        return;
    }
    rz_core_seek(core, offset, true);
}

void CutterCore::seek(ut64 offset)
{
    // Slower than using the API, but the API is not complete
    // which means we either have to duplicate code from rizin
    // here, or refactor rizin API.
    CORE_LOCK();
    if (offset == RVA_INVALID) {
        return;
    }

    RVA o_offset = core->offset;
    rz_core_seek_and_save(core, offset, true);
    if (o_offset != core->offset) {
        updateSeek();
    }
}

void CutterCore::showMemoryWidget()
{
    emit showMemoryWidgetRequested();
}

void CutterCore::seekAndShow(ut64 offset)
{
    seek(offset);
    showMemoryWidget();
}

void CutterCore::seekAndShow(QString offset)
{
    seek(offset);
    showMemoryWidget();
}

void CutterCore::seek(QString thing)
{
    CORE_LOCK();
    ut64 addr = rz_num_math(core->num, thing.toUtf8().constData());
    if (core->num->nc.errors) {
        return;
    }
    rz_core_seek_and_save(core, addr, true);
    updateSeek();
}

void CutterCore::seekPrev()
{
    CORE_LOCK();
    rz_core_seek_undo(core);
    updateSeek(SeekHistoryType::Undo);
}

void CutterCore::seekNext()
{
    CORE_LOCK();
    rz_core_seek_redo(core);
    updateSeek(SeekHistoryType::Redo);
}

void CutterCore::updateSeek(SeekHistoryType type)
{
    emit seekChanged(getOffset(), type);
}

RVA CutterCore::prevOpAddr(RVA startAddr, int count)
{
    CORE_LOCK();
    return rz_core_prevop_addr_force(core, startAddr, count);
}

RVA CutterCore::nextOpAddr(RVA startAddr, int count)
{
    CORE_LOCK();
    auto seek = seekTemp(startAddr);
    auto consumed =
            rz_core_analysis_ops_size(core, core->offset, core->block, (int)core->blocksize, count);

    RVA addr = startAddr + consumed;
    return addr;
}

RVA CutterCore::getOffset()
{
    return core_->offset;
}

void CutterCore::applySignature(const QString &filepath)
{
    CORE_LOCK();
    int old_cnt, new_cnt;
    const char *arch = rz_config_get(core->config, "asm.arch");
    ut8 expected_arch = rz_core_flirt_arch_from_name(arch);
    if (expected_arch == RZ_FLIRT_SIG_ARCH_ANY && filepath.endsWith(".sig", Qt::CaseInsensitive)) {
        QMessageBox::warning(nullptr, tr("Signatures"),
                             tr("Cannot apply signature file because the requested arch is not "
                                "supported by .sig "
                                "files"));
        return;
    }
    old_cnt = rz_flag_count(core->flags, "flirt");
    if (rz_sign_flirt_apply(core->analysis, filepath.toStdString().c_str(), expected_arch)) {
        new_cnt = rz_flag_count(core->flags, "flirt");
        QMessageBox::information(nullptr, tr("Signatures"),
                                 tr("Found %1 matching signatures!").arg(new_cnt - old_cnt));
        return;
    }
    QMessageBox::warning(
            nullptr, tr("Signatures"),
            tr("Failed to apply signature file!\nPlease check the console for more details."));
}

void CutterCore::createSignature(const QString &filepath)
{
    CORE_LOCK();
    ut32 n_modules = 0;
    if (!rz_core_flirt_create_file(core, filepath.toStdString().c_str(), &n_modules)) {
        QMessageBox::warning(
                nullptr, tr("Signatures"),
                tr("Cannot create signature file (check the console for more details)."));
        return;
    }
    QMessageBox::information(nullptr, tr("Signatures"),
                             tr("Written %1 signatures to %2.").arg(n_modules).arg(filepath));
}

ut64 CutterCore::math(const QString &expr)
{
    CORE_LOCK();
    return rz_num_math(core ? core->num : NULL, expr.toUtf8().constData());
}

ut64 CutterCore::num(const QString &expr)
{
    CORE_LOCK();
    return rz_num_get(core ? core->num : NULL, expr.toUtf8().constData());
}

QString CutterCore::itoa(ut64 num, int rdx)
{
    return QString::number(num, rdx);
}

void CutterCore::setConfig(const char *k, const char *v)
{
    CORE_LOCK();
    rz_config_set(core->config, k, v);
}

void CutterCore::setConfig(const QString &k, const char *v)
{
    CORE_LOCK();
    rz_config_set(core->config, k.toUtf8().constData(), v);
}

void CutterCore::setConfig(const char *k, const QString &v)
{
    CORE_LOCK();
    rz_config_set(core->config, k, v.toUtf8().constData());
}

void CutterCore::setConfig(const char *k, int v)
{
    CORE_LOCK();
    rz_config_set_i(core->config, k, static_cast<ut64>(v));
}

void CutterCore::setConfig(const char *k, bool v)
{
    CORE_LOCK();
    rz_config_set_i(core->config, k, v ? 1 : 0);
}

int CutterCore::getConfigi(const char *k)
{
    CORE_LOCK();
    return static_cast<int>(rz_config_get_i(core->config, k));
}

ut64 CutterCore::getConfigut64(const char *k)
{
    CORE_LOCK();
    return rz_config_get_i(core->config, k);
}

bool CutterCore::getConfigb(const char *k)
{
    CORE_LOCK();
    return rz_config_get_i(core->config, k) != 0;
}

QString CutterCore::getConfigDescription(const char *k)
{
    CORE_LOCK();
    RzConfigNode *node = rz_config_node_get(core->config, k);
    return node ? QString(node->desc) : QString("Unrecognized configuration key");
}

void CutterCore::triggerRefreshAll()
{
    emit refreshAll();
}

void CutterCore::triggerAsmOptionsChanged()
{
    emit asmOptionsChanged();
}

void CutterCore::triggerGraphOptionsChanged()
{
    emit graphOptionsChanged();
}

void CutterCore::message(const QString &msg, bool debug)
{
    if (msg.isEmpty())
        return;
    if (debug) {
        qDebug() << msg;
        emit newDebugMessage(msg);
        return;
    }
    emit newMessage(msg);
}

QString CutterCore::getConfig(const char *k)
{
    CORE_LOCK();
    return { rz_config_get(core->config, k) };
}

QStringList CutterCore::getConfigOptions(const char *k)
{
    CORE_LOCK();
    RzConfigNode *node = rz_config_node_get(core->config, k);
    if (!(node && node->options)) {
        return {};
    }
    QStringList list;
    for (const auto &s : CutterRzList<char>(node->options)) {
        list << s;
    }
    return list;
}

void CutterCore::setConfig(const char *k, const QVariant &v)
{
    switch (v.type()) {
    case QVariant::Type::Bool:
        setConfig(k, v.toBool());
        break;
    case QVariant::Type::Int:
        setConfig(k, v.toInt());
        break;
    default:
        setConfig(k, v.toString());
        break;
    }
}

void CutterCore::setCPU(QString arch, QString cpu, int bits)
{
    if (arch != nullptr) {
        setConfig("asm.arch", arch);
    }
    if (cpu != nullptr) {
        setConfig("asm.cpu", cpu);
    }
    setConfig("asm.bits", bits);
}

void CutterCore::setEndianness(bool big)
{
    setConfig("cfg.bigendian", big);
}

QByteArray CutterCore::assemble(const QString &code)
{
    CORE_LOCK();
    RzAsmCode *ac = rz_asm_massemble(core->rasm, code.toUtf8().constData());
    QByteArray res;
    if (ac && ac->bytes) {
        res = QByteArray(reinterpret_cast<const char *>(ac->bytes), ac->len);
    }
    rz_asm_code_free(ac);
    return res;
}

QString CutterCore::disassemble(const QByteArray &data)
{
    CORE_LOCK();
    RzAsmCode *ac = rz_asm_mdisassemble(core->rasm, reinterpret_cast<const ut8 *>(data.constData()),
                                        data.length());
    QString code;
    if (ac && ac->assembly) {
        code = QString::fromUtf8(ac->assembly);
    }
    rz_asm_code_free(ac);
    return code;
}

QString CutterCore::disassembleSingleInstruction(RVA addr)
{
    auto ab = getRzAnalysisBytesSingle(addr);
    return QString(ab->disasm).simplified();
}

RzAnalysisFunction *CutterCore::functionIn(ut64 addr)
{
    CORE_LOCK();
    RzAnalysisFunction *fcn = rz_analysis_get_function_at(core->analysis, addr);
    if (fcn) {
        return fcn;
    }
    RzList *fcns = rz_analysis_get_functions_in(core->analysis, addr);
    fcn = !rz_list_empty(fcns) ? reinterpret_cast<RzAnalysisFunction *>(rz_list_first(fcns))
                               : nullptr;
    rz_list_free(fcns);
    return fcn;
}

RzAnalysisFunction *CutterCore::functionAt(ut64 addr)
{
    CORE_LOCK();
    return rz_analysis_get_function_at(core->analysis, addr);
}

/**
 * @brief finds the start address of a function in a given address
 * @param addr - an address which belongs to a function
 * @returns if function exists, return its start address. Otherwise return RVA_INVALID
 */
RVA CutterCore::getFunctionStart(RVA addr)
{
    CORE_LOCK();
    RzAnalysisFunction *fcn = Core()->functionIn(addr);
    return fcn ? fcn->addr : RVA_INVALID;
}

/**
 * @brief finds the end address of a function in a given address
 * @param addr - an address which belongs to a function
 * @returns if function exists, return its end address. Otherwise return RVA_INVALID
 */
RVA CutterCore::getFunctionEnd(RVA addr)
{
    CORE_LOCK();
    RzAnalysisFunction *fcn = Core()->functionIn(addr);
    return fcn ? fcn->addr : RVA_INVALID;
}

/**
 * @brief finds the last instruction of a function in a given address
 * @param addr - an address which belongs to a function
 * @returns if function exists, return the address of its last instruction. Otherwise return
 * RVA_INVALID
 */
RVA CutterCore::getLastFunctionInstruction(RVA addr)
{
    CORE_LOCK();
    RzAnalysisFunction *fcn = Core()->functionIn(addr);
    if (!fcn) {
        return RVA_INVALID;
    }
    RzAnalysisBlock *lastBB = (RzAnalysisBlock *)rz_pvector_tail(fcn->bbs);
    return lastBB ? rz_analysis_block_get_op_addr(lastBB, lastBB->ninstr - 1) : RVA_INVALID;
}

QString CutterCore::flagAt(RVA addr, bool getClosestFlag)
{
    CORE_LOCK();
    // rz_flag_get_at and rz_flag_get_i can return different
    // flags for addresses containing multiple flags, so we must use rz_flag_get_i here
    // instead of setting rz_flag_get_at's "closest" argument to false
    RzFlagItem *f = getClosestFlag ? rz_flag_get_at(core->flags, addr, true)
                                   : rz_flag_get_i(core->flags, addr);
    if (!f) {
        return {};
    }
    return core->flags->realnames && f->realname ? f->realname : f->name;
}

void CutterCore::createFunctionAt(RVA addr)
{
    createFunctionAt(addr, "");
}

void CutterCore::createFunctionAt(RVA addr, QString name)
{
    if (!name.isEmpty() && !name.isNull()) {
        static const QRegularExpression regExp("[^a-zA-Z0-9_.]");
        name.remove(regExp);
    }

    CORE_LOCK();
    bool analyze_recursively = rz_config_get_i(core->config, "analysis.calls");
    rz_core_analysis_function_add(core, name.toStdString().c_str(), addr, analyze_recursively);
    emit functionsChanged();
}

RVA CutterCore::getOffsetJump(RVA addr)
{
    auto ab = getRzAnalysisBytesSingle(addr);
    return ab && ab->op ? ab->op->jump : RVA_INVALID;
}

QList<Decompiler *> CutterCore::getDecompilers()
{
    return decompilers;
}

Decompiler *CutterCore::getDecompilerById(const QString &id)
{
    for (Decompiler *dec : decompilers) {
        if (dec->getId() == id) {
            return dec;
        }
    }
    return nullptr;
}

bool CutterCore::registerDecompiler(Decompiler *decompiler)
{
    if (getDecompilerById(decompiler->getId())) {
        return false;
    }
    decompiler->setParent(this);
    decompilers.push_back(decompiler);
    return true;
}

CutterJson CutterCore::getSignatureInfo()
{
    CORE_LOCK();
    RzBinFile *cur = rz_bin_cur(core->bin);
    RzBinPlugin *plg = rz_bin_file_cur_plugin(cur);
    if (!plg || !plg->signature) {
        return {};
    }
    char *signature = plg->signature(cur, true);
    if (!signature) {
        return {};
    }
    return parseJson("signature", signature, nullptr);
}

bool CutterCore::existsFileInfo()
{
    CORE_LOCK();
    RzBinObject *bobj = rz_bin_cur_object(core->bin);
    if (!bobj) {
        return false;
    }
    const RzBinInfo *info = rz_bin_object_get_info(bobj);
    if (!(info && info->rclass)) {
        return false;
    }
    return strncmp("pe", info->rclass, 2) == 0 || strncmp("elf", info->rclass, 3) == 0;
}

// Utility function to check if a telescoped item exists and add it with prefixes to the desc
static inline const QString appendVar(QString &dst, const QString val, const QString prepend_val,
                                      const QString append_val)
{
    if (!val.isEmpty()) {
        dst += prepend_val + val + append_val;
    }
    return val;
}

RefDescription CutterCore::formatRefDesc(const QSharedPointer<AddrRefs> &refItem)
{
    RefDescription desc;

    if (refItem->addr == RVA_INVALID) {
        return desc;
    }

    QString str = refItem->string;
    if (!str.isEmpty()) {
        desc.ref = str;
        desc.refColor = ConfigColor("comment");
    } else {
        QSharedPointer<const AddrRefs> cursor(refItem);
        QString type, string;
        while (true) {
            desc.ref += " ->";
            appendVar(desc.ref, cursor->reg, " @", "");
            appendVar(desc.ref, cursor->mapname, " (", ")");
            appendVar(desc.ref, cursor->section, " (", ")");
            appendVar(desc.ref, cursor->fcn, " ", "");
            type = appendVar(desc.ref, cursor->type, " ", "");
            appendVar(desc.ref, cursor->perms, " ", "");
            appendVar(desc.ref, cursor->asm_op, " \"", "\"");
            string = appendVar(desc.ref, cursor->string, " ", "");
            if (!string.isNull()) {
                // There is no point in adding ascii and addr info after a string
                break;
            }
            if (cursor->has_value) {
                appendVar(desc.ref, RzAddressString(cursor->value), " ", "");
            }
            if (!cursor->ref) {
                break;
            }
            cursor = cursor->ref;
        }

        // Set the ref's color according to the last item type
        if (type == "ascii" || !string.isEmpty()) {
            desc.refColor = ConfigColor("comment");
        } else if (type == "program") {
            desc.refColor = ConfigColor("fname");
        } else if (type == "library") {
            desc.refColor = ConfigColor("floc");
        } else if (type == "stack") {
            desc.refColor = ConfigColor("offset");
        }
    }

    return desc;
}

RzReg *CutterCore::getReg()
{
    CORE_LOCK();
    if (currentlyDebugging && currentlyEmulating) {
        return core->analysis->reg;
    } else if (currentlyDebugging) {
        return core->dbg->reg;
    }
    return core->analysis->reg;
}

QList<RegisterRef> CutterCore::getRegisterRefs(int depth)
{
    QList<RegisterRef> ret;
    if (!currentlyDebugging) {
        return ret;
    }

    CORE_LOCK();
    RzList *ritems = rz_core_reg_filter_items_sync(core, getReg(), reg_sync, nullptr);
    if (!ritems) {
        return ret;
    }
    RzListIter *it;
    RzRegItem *ri;
    CutterRzListForeach (ritems, it, RzRegItem, ri) {
        RegisterRef reg;
        reg.value = rz_reg_get_value(getReg(), ri);
        reg.ref = getAddrRefs(reg.value, depth);
        reg.name = ri->name;
        ret.append(reg);
    }
    rz_list_free(ritems);
    return ret;
}

QList<AddrRefs> CutterCore::getStack(int size, int depth)
{
    QList<AddrRefs> stack;
    if (!currentlyDebugging) {
        return stack;
    }

    CORE_LOCK();
    RVA addr = rz_core_reg_getv_by_role_or_name(core, "SP");
    if (addr == RVA_INVALID) {
        return stack;
    }

    int base = core->analysis->bits;
    for (int i = 0; i < size; i += base / 8) {
        if ((base == 32 && addr + i >= UT32_MAX) || (base == 16 && addr + i >= UT16_MAX)) {
            break;
        }

        stack.append(getAddrRefs(addr + i, depth));
    }

    return stack;
}

AddrRefs CutterCore::getAddrRefs(RVA addr, int depth)
{
    AddrRefs refs;
    if (depth < 1 || addr == UT64_MAX) {
        refs.addr = RVA_INVALID;
        return refs;
    }

    CORE_LOCK();
    int bits = core->rasm->bits;
    QByteArray buf = QByteArray();
    ut64 type = rz_core_analysis_address(core, addr);

    refs.addr = addr;

    // Search for the section the addr is in, avoid duplication for heap/stack with type
    if (!(type & RZ_ANALYSIS_ADDR_TYPE_HEAP || type & RZ_ANALYSIS_ADDR_TYPE_STACK)) {
        // Attempt to find the address within a map
        RzDebugMap *map = rz_debug_map_get(core->dbg, addr);
        if (map && map->name && map->name[0]) {
            refs.mapname = map->name;
        }

        RzBinSection *sect = rz_bin_get_section_at(rz_bin_cur_object(core->bin), addr, true);
        if (sect && sect->name[0]) {
            refs.section = sect->name;
        }
    }

    // Check if the address points to a register
    RzFlagItem *fi = rz_flag_get_i(core->flags, addr);
    if (fi) {
        RzRegItem *r = rz_reg_get(getReg(), fi->name, -1);
        if (r) {
            refs.reg = r->name;
        }
    }

    // Attempt to find the address within a function
    RzAnalysisFunction *fcn = rz_analysis_get_fcn_in(core->analysis, addr, 0);
    if (fcn) {
        refs.fcn = fcn->name;
    }

    // Update type and permission information
    if (type != 0) {
        if (type & RZ_ANALYSIS_ADDR_TYPE_HEAP) {
            refs.type = "heap";
        } else if (type & RZ_ANALYSIS_ADDR_TYPE_STACK) {
            refs.type = "stack";
        } else if (type & RZ_ANALYSIS_ADDR_TYPE_PROGRAM) {
            refs.type = "program";
        } else if (type & RZ_ANALYSIS_ADDR_TYPE_LIBRARY) {
            refs.type = "library";
        } else if (type & RZ_ANALYSIS_ADDR_TYPE_ASCII) {
            refs.type = "ascii";
        } else if (type & RZ_ANALYSIS_ADDR_TYPE_SEQUENCE) {
            refs.type = "sequence";
        }

        QString perms = "";
        if (type & RZ_ANALYSIS_ADDR_TYPE_READ) {
            perms += "r";
        }
        if (type & RZ_ANALYSIS_ADDR_TYPE_WRITE) {
            perms += "w";
        }
        if (type & RZ_ANALYSIS_ADDR_TYPE_EXEC) {
            RzAsmOp op;
            buf.resize(32);
            perms += "x";
            // Instruction disassembly
            rz_io_read_at(core->io, addr, (unsigned char *)buf.data(), buf.size());
            rz_asm_set_pc(core->rasm, addr);
            rz_asm_disassemble(core->rasm, &op, (unsigned char *)buf.data(), buf.size());
            refs.asm_op = rz_asm_op_get_asm(&op);
        }

        if (!perms.isEmpty()) {
            refs.perms = perms;
        }
    }

    // Try to telescope further if depth permits it
    if ((type & RZ_ANALYSIS_ADDR_TYPE_READ)) {
        buf.resize(64);
        ut32 *n32 = (ut32 *)buf.data();
        ut64 *n64 = (ut64 *)buf.data();
        rz_io_read_at(core->io, addr, (unsigned char *)buf.data(), buf.size());
        ut64 n = (bits == 64) ? *n64 : *n32;
        // The value of the next address will serve as an indication that there's more to
        // telescope if we have reached the depth limit
        refs.value = n;
        refs.has_value = true;
        if (depth && n != addr && !(type & RZ_ANALYSIS_ADDR_TYPE_EXEC)) {
            // Make sure we aren't telescoping the same address
            AddrRefs ref = getAddrRefs(n, depth - 1);
            if (!ref.type.isNull()) {
                // If the dereference of the current pointer is an ascii character we
                // might have a string in this address
                if (ref.type.contains("ascii")) {
                    buf.resize(128);
                    rz_io_read_at(core->io, addr, (unsigned char *)buf.data(), buf.size());
                    QString strVal = QString(buf);
                    // Indicate that the string is longer than the printed value
                    if (strVal.size() == buf.size()) {
                        strVal += "...";
                    }
                    refs.string = strVal;
                }
                refs.ref = QSharedPointer<AddrRefs>::create(ref);
            }
        }
    }
    return refs;
}

QVector<Chunk> CutterCore::getHeapChunks(RVA arena_addr)
{
    CORE_LOCK();
    QVector<Chunk> chunks_vector;
    ut64 m_arena;

    if (!arena_addr) {
        // if arena_addr is zero get base address of main arena
        RzList *arenas = rz_heap_arenas_list(core);
        if (arenas->length == 0) {
            rz_list_free(arenas);
            return chunks_vector;
        }
        m_arena = ((RzArenaListItem *)rz_list_first(arenas))->addr;
        rz_list_free(arenas);
    } else {
        m_arena = arena_addr;
    }

    // Get chunks using api and store them in a chunks_vector
    RzList *chunks = rz_heap_chunks_list(core, m_arena);
    RzListIter *iter;
    RzHeapChunkListItem *data;
    CutterRzListForeach (chunks, iter, RzHeapChunkListItem, data) {
        Chunk chunk;
        chunk.offset = data->addr;
        chunk.size = (int)data->size;
        chunk.status = QString(data->status);
        chunks_vector.append(chunk);
    }

    rz_list_free(chunks);
    return chunks_vector;
}

int CutterCore::getArchBits()
{
    CORE_LOCK();
    return core->dbg->bits;
}

QVector<Arena> CutterCore::getArenas()
{
    CORE_LOCK();
    QVector<Arena> arena_vector;

    // get arenas using API and store them in arena_vector
    RzList *arenas = rz_heap_arenas_list(core);
    RzListIter *iter;
    RzArenaListItem *data;
    CutterRzListForeach (arenas, iter, RzArenaListItem, data) {
        Arena arena;
        arena.offset = data->addr;
        arena.type = QString(data->type);
        arena.last_remainder = data->arena->last_remainder;
        arena.top = data->arena->top;
        arena.next = data->arena->next;
        arena.next_free = data->arena->next_free;
        arena.system_mem = data->arena->system_mem;
        arena.max_system_mem = data->arena->max_system_mem;
        arena_vector.append(arena);
    }

    rz_list_free(arenas);
    return arena_vector;
}

RzHeapChunkSimple *CutterCore::getHeapChunk(ut64 addr)
{
    CORE_LOCK();
    return rz_heap_chunk(core, addr);
}

QVector<RzHeapBin *> CutterCore::getHeapBins(ut64 arena_addr)
{
    CORE_LOCK();
    QVector<RzHeapBin *> bins_vector;

    MallocState *arena = rz_heap_get_arena(core, arena_addr);
    if (!arena) {
        return bins_vector;
    }

    // get small, large, unsorted bins
    for (int i = 0; i <= NBINS - 2; i++) {
        RzHeapBin *bin = rz_heap_bin_content(core, arena, i, arena_addr);
        if (!bin) {
            continue;
        }
        if (!rz_list_length(bin->chunks)) {
            rz_heap_bin_free_64(bin);
            continue;
        }
        bins_vector.append(bin);
    }
    // get fastbins
    for (int i = 0; i < 10; i++) {
        RzHeapBin *bin = rz_heap_fastbin_content(core, arena, i);
        if (!bin) {
            continue;
        }
        if (!rz_list_length(bin->chunks)) {
            rz_heap_bin_free_64(bin);
            continue;
        }
        bins_vector.append(bin);
    }
    // get tcache bins
    RzList *tcache_bins = rz_heap_tcache_content(core, arena_addr);
    RzListIter *iter;
    RzHeapBin *bin;
    CutterRzListForeach (tcache_bins, iter, RzHeapBin, bin) {
        if (!bin) {
            continue;
        }
        if (!rz_list_length(bin->chunks)) {
            rz_heap_bin_free_64(bin);
            continue;
        }
        bins_vector.append(bin);
    }
    return bins_vector;
}

bool CutterCore::writeHeapChunk(RzHeapChunkSimple *chunk_simple)
{
    CORE_LOCK();
    return rz_heap_write_chunk(core, chunk_simple);
}

QList<VariableDescription> CutterCore::getVariables(RVA at)
{
    QList<VariableDescription> ret;
    CORE_LOCK();
    RzAnalysisFunction *fcn = functionIn(at);
    if (!fcn) {
        return ret;
    }
    for (auto var : CutterPVector<RzAnalysisVar>(&fcn->vars)) {
        VariableDescription desc;
        desc.storageType = var->storage.type;
        if (!var->name || !var->type) {
            continue;
        }
        desc.name = QString::fromUtf8(var->name);
        char *tn = rz_type_as_string(core->analysis->typedb, var->type);
        if (!tn) {
            continue;
        }
        desc.type = QString::fromUtf8(tn);
        rz_mem_free(tn);

        if (char *v = rz_core_analysis_var_display(core, var, false)) {
            desc.value = QString::fromUtf8(v).trimmed();
            rz_mem_free(v);
        }

        ret.push_back(desc);
    }
    return ret;
}

QList<GlobalDescription> CutterCore::getAllGlobals()
{
    CORE_LOCK();
    RzListIter *it;

    QList<GlobalDescription> ret;

    RzAnalysisVarGlobal *glob;
    if (core && core->analysis && core->analysis->typedb) {
        const RzList *globals = rz_analysis_var_global_get_all(core->analysis);
        CutterRzListForeach (globals, it, RzAnalysisVarGlobal, glob) {
            const char *gtype = rz_type_as_string(core->analysis->typedb, glob->type);
            if (!gtype) {
                continue;
            }
            GlobalDescription global;
            global.addr = glob->addr;
            global.name = QString(glob->name);
            global.type = QString(gtype);
            ret << global;
        }
    }

    return ret;
}

QVector<RegisterRefValueDescription> CutterCore::getRegisterRefValues()
{
    QVector<RegisterRefValueDescription> result;
    CORE_LOCK();
    RzList *ritems = rz_core_reg_filter_items_sync(core, getReg(), reg_sync, nullptr);
    if (!ritems) {
        return result;
    }
    RzListIter *it;
    RzRegItem *ri;
    CutterRzListForeach (ritems, it, RzRegItem, ri) {
        RegisterRefValueDescription desc;
        desc.name = ri->name;
        ut64 value = rz_reg_get_value(getReg(), ri);
        desc.value = "0x" + QString::number(value, 16);
        desc.ref = rz_core_analysis_hasrefs(core, value, true);
        result.push_back(desc);
    }
    rz_list_free(ritems);
    return result;
}

QString CutterCore::getRegisterName(QString registerRole)
{
    if (!currentlyDebugging) {
        return "";
    }
    CORE_LOCK();
    return rz_reg_get_name_by_type(getReg(), registerRole.toUtf8().constData());
}

RVA CutterCore::getProgramCounterValue()
{
    if (currentlyDebugging) {
        CORE_LOCK();
        return rz_core_reg_getv_by_role_or_name(core, "PC");
    }
    return RVA_INVALID;
}

void CutterCore::setRegister(QString regName, QString regValue)
{
    if (!currentlyDebugging) {
        return;
    }
    CORE_LOCK();
    ut64 val = rz_num_math(core->num, regValue.toUtf8().constData());
    rz_core_reg_assign_sync(core, getReg(), reg_sync, regName.toUtf8().constData(), val);
    emit registersChanged();
    emit refreshCodeViews();
}

void CutterCore::setCurrentDebugThread(int tid)
{
    if (!asyncTask(
                [=](RzCore *core) {
                    rz_debug_select(core->dbg, core->dbg->pid, tid);
                    return (void *)NULL;
                },
                debugTask)) {
        return;
    }

    emit debugTaskStateChanged();
    connect(debugTask.data(), &RizinTask::finished, this, [this]() {
        debugTask.clear();
        emit registersChanged();
        emit refreshCodeViews();
        emit stackChanged();
        syncAndSeekProgramCounter();
        emit switchedThread();
        emit debugTaskStateChanged();
    });

    debugTask->startTask();
}

void CutterCore::setCurrentDebugProcess(int pid)
{
    if (!currentlyDebugging
        || !asyncTask(
                [=](RzCore *core) {
                    rz_debug_select(core->dbg, pid, core->dbg->tid);
                    core->dbg->main_pid = pid;
                    return (void *)NULL;
                },
                debugTask)) {
        return;
    }

    emit debugTaskStateChanged();
    connect(debugTask.data(), &RizinTask::finished, this, [this]() {
        debugTask.clear();
        emit registersChanged();
        emit refreshCodeViews();
        emit stackChanged();
        emit flagsChanged();
        syncAndSeekProgramCounter();
        emit switchedProcess();
        emit debugTaskStateChanged();
    });

    debugTask->startTask();
}

void CutterCore::startDebug()
{
    if (!currentlyDebugging) {
        offsetPriorDebugging = getOffset();
    }
    currentlyOpenFile = getConfig("file.path");

    if (!asyncTask(
                [](RzCore *core) {
                    rz_core_file_reopen_debug(core, "");
                    return nullptr;
                },
                debugTask)) {
        return;
    }

    emit debugTaskStateChanged();

    connect(debugTask.data(), &RizinTask::finished, this, [this]() {
        delete debugTaskDialog;
        debugTask.clear();

        emit registersChanged();
        if (!currentlyDebugging) {
            setConfig("asm.flags", false);
            currentlyDebugging = true;
            emit toggleDebugView();
            emit refreshCodeViews();
        }

        emit codeRebased();
        emit stackChanged();
        emit debugTaskStateChanged();
    });

    debugTaskDialog = new RizinTaskDialog(debugTask);
    debugTaskDialog->setBreakOnClose(true);
    debugTaskDialog->setAttribute(Qt::WA_DeleteOnClose);
    debugTaskDialog->setDesc(tr("Starting native debug..."));
    debugTaskDialog->show();

    debugTask->startTask();
}

void CutterCore::startEmulation()
{
    if (!currentlyDebugging) {
        offsetPriorDebugging = getOffset();
    }

    // clear registers, init esil state, stack, progcounter at current seek
    asyncTask(
            [&](RzCore *core) {
                rz_core_analysis_esil_reinit(core);
                rz_core_analysis_esil_init_mem(core, NULL, UT64_MAX, UT32_MAX);
                rz_core_analysis_esil_init_regs(core);
                return nullptr;
            },
            debugTask);

    emit debugTaskStateChanged();

    connect(debugTask.data(), &RizinTask::finished, this, [this]() {
        delete debugTaskDialog;
        debugTask.clear();

        if (!currentlyDebugging || !currentlyEmulating) {
            // prevent register flags from appearing during debug/emul
            setConfig("asm.flags", false);
            // allows to view self-modifying code changes or other binary changes
            setConfig("io.cache", true);
            currentlyDebugging = true;
            currentlyEmulating = true;
            emit toggleDebugView();
        }

        emit registersChanged();
        emit stackChanged();
        emit codeRebased();
        emit refreshCodeViews();
        emit debugTaskStateChanged();
    });

    debugTaskDialog = new RizinTaskDialog(debugTask);
    debugTaskDialog->setBreakOnClose(true);
    debugTaskDialog->setAttribute(Qt::WA_DeleteOnClose);
    debugTaskDialog->setDesc(tr("Starting emulation..."));
    debugTaskDialog->show();

    debugTask->startTask();
}

void CutterCore::attachRemote(const QString &uri)
{
    if (!currentlyDebugging) {
        offsetPriorDebugging = getOffset();
    }

    // connect to a debugger with the given plugin
    if (!asyncTask(
                [&](RzCore *core) {
                    setConfig("cfg.debug", true);
                    rz_core_file_reopen_remote_debug(core, uri.toStdString().c_str(), 0);
                    return nullptr;
                },
                debugTask)) {
        return;
    }
    emit debugTaskStateChanged();

    connect(debugTask.data(), &RizinTask::finished, this, [this, uri]() {
        delete debugTaskDialog;
        debugTask.clear();
        // Check if we actually connected
        bool connected = false;
        RzCoreLocked core(Core());
        RzList *descs = rz_id_storage_list(core->io->files);
        RzListIter *it;
        RzIODesc *desc;
        CutterRzListForeach (descs, it, RzIODesc, desc) {
            QString fileUri = QString(desc->uri);
            if (!fileUri.compare(uri)) {
                connected = true;
            }
        }
        seekAndShow(getProgramCounterValue());
        if (!connected) {
            emit attachedRemote(false);
            emit debugTaskStateChanged();
            return;
        }

        emit registersChanged();
        if (!currentlyDebugging || !currentlyEmulating) {
            // prevent register flags from appearing during debug/emul
            setConfig("asm.flags", false);
            currentlyDebugging = true;
            emit toggleDebugView();
        }

        currentlyRemoteDebugging = true;
        emit codeRebased();
        emit attachedRemote(true);
        emit debugTaskStateChanged();
    });

    debugTaskDialog = new RizinTaskDialog(debugTask);
    debugTaskDialog->setBreakOnClose(true);
    debugTaskDialog->setAttribute(Qt::WA_DeleteOnClose);
    debugTaskDialog->setDesc(tr("Connecting to: ") + uri);
    debugTaskDialog->show();

    debugTask->startTask();
}

void CutterCore::attachDebug(int pid)
{
    if (!currentlyDebugging) {
        offsetPriorDebugging = getOffset();
    }

    if (!asyncTask(
                [&](RzCore *core) {
                    // cannot use setConfig because core is
                    // already locked, which causes a deadlock
                    rz_config_set_b(core->config, "cfg.debug", true);
                    auto uri = rz_str_newf("dbg://%d", pid);
                    if (currentlyOpenFile.isEmpty()) {
                        rz_core_file_open_load(core, uri, 0, RZ_PERM_R, false);
                    } else {
                        rz_core_file_reopen_remote_debug(core, uri, 0);
                    }
                    free(uri);
                    return nullptr;
                },
                debugTask)) {
        return;
    }
    emit debugTaskStateChanged();

    connect(debugTask.data(), &RizinTask::finished, this, [this, pid]() {
        delete debugTaskDialog;
        debugTask.clear();

        syncAndSeekProgramCounter();
        if (!currentlyDebugging || !currentlyEmulating) {
            // prevent register flags from appearing during debug/emul
            setConfig("asm.flags", false);
            currentlyDebugging = true;
            currentlyOpenFile = getConfig("file.path");
            currentlyAttachedToPID = pid;
            emit toggleDebugView();
        }

        emit codeRebased();
        emit debugTaskStateChanged();
    });

    debugTaskDialog = new RizinTaskDialog(debugTask);
    debugTaskDialog->setBreakOnClose(true);
    debugTaskDialog->setAttribute(Qt::WA_DeleteOnClose);
    debugTaskDialog->setDesc(tr("Attaching to process (") + QString::number(pid) + ")...");
    debugTaskDialog->show();

    debugTask->startTask();
}

void CutterCore::suspendDebug()
{
    debugTask->breakTask();
    debugTask->joinTask();
}

void CutterCore::stopDebug()
{
    if (!currentlyDebugging) {
        return;
    }

    if (!debugTask.isNull()) {
        suspendDebug();
    }

    currentlyDebugging = false;
    currentlyTracing = false;
    currentlyRemoteDebugging = false;
    emit debugTaskStateChanged();

    CORE_LOCK();
    if (currentlyEmulating) {
        rz_core_analysis_esil_init_mem_del(core, NULL, UT64_MAX, UT32_MAX);
        rz_core_analysis_esil_deinit(core);
        resetWriteCache();
        rz_core_debug_clear_register_flags(core);
        rz_core_analysis_esil_trace_stop(core);
        currentlyEmulating = false;
    } else {
        // ensure we have opened a file.
        if (core->io->desc) {
            rz_core_debug_process_close(core);
        }
        currentlyAttachedToPID = -1;
    }

    syncAndSeekProgramCounter();
    setConfig("asm.flags", true);
    setConfig("io.cache", false);
    emit codeRebased();
    emit toggleDebugView();
    offsetPriorDebugging = getOffset();
    emit debugTaskStateChanged();
}

void CutterCore::syncAndSeekProgramCounter()
{
    seekAndShow(getProgramCounterValue());
    emit registersChanged();
}

void CutterCore::continueDebug()
{
    if (!currentlyDebugging) {
        return;
    }

    if (currentlyEmulating) {
        if (!asyncTask(
                    [](RzCore *core) {
                        rz_core_esil_step(core, UT64_MAX, "0", NULL, false);
                        rz_core_reg_update_flags(core);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    } else {
        if (!asyncTask(
                    [](RzCore *core) {
                        rz_debug_continue(core->dbg);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    }

    emit debugTaskStateChanged();
    connect(debugTask.data(), &RizinTask::finished, this, [this]() {
        debugTask.clear();
        syncAndSeekProgramCounter();
        emit refreshCodeViews();
        emit debugTaskStateChanged();
    });

    debugTask->startTask();
}

void CutterCore::continueBackDebug()
{
    if (!currentlyDebugging) {
        return;
    }

    if (currentlyEmulating) {
        if (!asyncTask(
                    [](RzCore *core) {
                        rz_core_esil_continue_back(core);
                        rz_core_reg_update_flags(core);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    } else {
        if (!asyncTask(
                    [](RzCore *core) {
                        rz_debug_continue_back(core->dbg);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    }
    emit debugTaskStateChanged();

    connect(debugTask.data(), &RizinTask::finished, this, [this]() {
        debugTask.clear();
        syncAndSeekProgramCounter();
        emit refreshCodeViews();
        emit debugTaskStateChanged();
    });

    debugTask->startTask();
}

void CutterCore::continueUntilDebug(ut64 offset)
{
    if (!currentlyDebugging) {
        return;
    }

    if (currentlyEmulating) {
        if (!asyncTask(
                    [=](RzCore *core) {
                        rz_core_esil_step(core, offset, NULL, NULL, false);
                        rz_core_reg_update_flags(core);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    } else {
        if (!asyncTask(
                    [=](RzCore *core) {
                        rz_core_debug_continue_until(core, offset);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    }
    emit debugTaskStateChanged();
    connect(debugTask.data(), &RizinTask::finished, this, [this]() {
        debugTask.clear();
        syncAndSeekProgramCounter();
        emit refreshCodeViews();
        emit debugTaskStateChanged();
    });
    debugTask->startTask();
}

void CutterCore::continueUntilCall()
{
    if (!currentlyDebugging) {
        return;
    }

    if (currentlyEmulating) {
        if (!asyncTask(
                    [](RzCore *core) {
                        rz_core_analysis_continue_until_call(core);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    } else {
        if (!asyncTask(
                    [](RzCore *core) {
                        rz_core_debug_step_one(core, 0);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    }

    emit debugTaskStateChanged();
    connect(debugTask.data(), &RizinTask::finished, this, [this]() {
        debugTask.clear();
        syncAndSeekProgramCounter();
        emit refreshCodeViews();
        emit debugTaskStateChanged();
    });

    debugTask->startTask();
}

void CutterCore::continueUntilSyscall()
{
    if (!currentlyDebugging) {
        return;
    }

    if (currentlyEmulating) {
        if (!asyncTask(
                    [](RzCore *core) {
                        rz_core_analysis_continue_until_syscall(core);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    } else {
        if (!asyncTask(
                    [](RzCore *core) {
                        rz_cons_break_push(
                                [](void *x) { rz_debug_stop(reinterpret_cast<RzDebug *>(x)); },
                                core->dbg);
                        rz_reg_arena_swap(core->dbg->reg, true);
                        rz_debug_continue_syscalls(core->dbg, NULL, 0);
                        rz_cons_break_pop();
                        rz_core_dbg_follow_seek_register(core);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    }

    emit debugTaskStateChanged();
    connect(debugTask.data(), &RizinTask::finished, this, [this]() {
        debugTask.clear();
        syncAndSeekProgramCounter();
        emit refreshCodeViews();
        emit debugTaskStateChanged();
    });

    debugTask->startTask();
}

void CutterCore::stepDebug()
{
    if (!currentlyDebugging) {
        return;
    }

    if (currentlyEmulating) {
        if (!asyncTask(
                    [](RzCore *core) {
                        rz_core_esil_step(core, UT64_MAX, NULL, NULL, false);
                        rz_core_reg_update_flags(core);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    } else {
        if (!asyncTask(
                    [](RzCore *core) {
                        rz_core_debug_step_one(core, 1);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    }

    emit debugTaskStateChanged();
    connect(debugTask.data(), &RizinTask::finished, this, [this]() {
        debugTask.clear();
        syncAndSeekProgramCounter();
        emit refreshCodeViews();
        emit debugTaskStateChanged();
    });

    debugTask->startTask();
}

void CutterCore::stepOverDebug()
{
    if (!currentlyDebugging) {
        return;
    }

    if (currentlyEmulating) {
        if (!asyncTask(
                    [&](RzCore *core) {
                        rz_core_analysis_esil_step_over(core);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    } else {
        asyncTask(
                [](RzCore *core) {
                    rz_core_debug_step_over(core, 1);
                    rz_core_dbg_follow_seek_register(core);
                    return nullptr;
                },
                debugTask);
    }

    emit debugTaskStateChanged();
    connect(debugTask.data(), &RizinTask::finished, this, [this]() {
        debugTask.clear();
        syncAndSeekProgramCounter();
        emit refreshCodeViews();
        emit debugTaskStateChanged();
    });

    debugTask->startTask();
}

void CutterCore::stepOutDebug()
{
    if (!currentlyDebugging) {
        return;
    }

    emit debugTaskStateChanged();
    asyncTask(
            [](RzCore *core) {
                rz_core_debug_step_until_frame(core);
                rz_core_dbg_follow_seek_register(core);
                return nullptr;
            },
            debugTask);

    connect(debugTask.data(), &RizinTask::finished, this, [this]() {
        debugTask.clear();
        syncAndSeekProgramCounter();
        emit refreshCodeViews();
        emit debugTaskStateChanged();
    });

    debugTask->startTask();
}

void CutterCore::stepBackDebug()
{
    if (!currentlyDebugging) {
        return;
    }

    if (currentlyEmulating) {
        if (!asyncTask(
                    [](RzCore *core) {
                        rz_core_esil_step_back(core);
                        rz_core_reg_update_flags(core);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    } else {
        asyncTask(
                [](RzCore *core) {
                    rz_core_debug_step_back(core, 1);
                    rz_core_dbg_follow_seek_register(core);
                    return nullptr;
                },
                debugTask);
    }
    emit debugTaskStateChanged();

    connect(debugTask.data(), &RizinTask::finished, this, [this]() {
        debugTask.clear();
        syncAndSeekProgramCounter();
        emit refreshCodeViews();
        emit debugTaskStateChanged();
    });

    debugTask->startTask();
}

QStringList CutterCore::getDebugPlugins()
{
    QStringList plugins;
    CORE_LOCK();
    CutterHtSP<RzDebugPlugin>(core->dbg->plugins)
            .ForEach([&plugins](const char *k, const RzDebugPlugin *plugin) {
                plugins << plugin->name;
                return true;
            });
    return plugins;
}

QString CutterCore::getActiveDebugPlugin()
{
    return getConfig("dbg.backend");
}

void CutterCore::setDebugPlugin(QString plugin)
{
    setConfig("dbg.backend", plugin);
}

void CutterCore::startTraceSession()
{
    if (!currentlyDebugging || currentlyTracing) {
        return;
    }

    if (currentlyEmulating) {
        if (!asyncTask(
                    [](RzCore *core) {
                        rz_core_analysis_esil_trace_start(core);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    } else {
        if (!asyncTask(
                    [](RzCore *core) {
                        core->dbg->session = rz_debug_session_new();
                        rz_debug_add_checkpoint(core->dbg);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    }
    emit debugTaskStateChanged();

    connect(debugTask.data(), &RizinTask::finished, this, [this]() {
        delete debugTaskDialog;
        debugTask.clear();

        currentlyTracing = true;
        emit debugTaskStateChanged();
    });

    debugTaskDialog = new RizinTaskDialog(debugTask);
    debugTaskDialog->setBreakOnClose(true);
    debugTaskDialog->setAttribute(Qt::WA_DeleteOnClose);
    debugTaskDialog->setDesc(tr("Creating debug tracepoint..."));
    debugTaskDialog->show();

    debugTask->startTask();
}

void CutterCore::stopTraceSession()
{
    if (!currentlyDebugging || !currentlyTracing) {
        return;
    }

    if (currentlyEmulating) {
        if (!asyncTask(
                    [](RzCore *core) {
                        rz_core_analysis_esil_trace_stop(core);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    } else {
        if (!asyncTask(
                    [](RzCore *core) {
                        rz_debug_session_free(core->dbg->session);
                        core->dbg->session = NULL;
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    }
    emit debugTaskStateChanged();

    connect(debugTask.data(), &RizinTask::finished, this, [this]() {
        delete debugTaskDialog;
        debugTask.clear();

        currentlyTracing = false;
        emit debugTaskStateChanged();
    });

    debugTaskDialog = new RizinTaskDialog(debugTask);
    debugTaskDialog->setBreakOnClose(true);
    debugTaskDialog->setAttribute(Qt::WA_DeleteOnClose);
    debugTaskDialog->setDesc(tr("Stopping debug session..."));
    debugTaskDialog->show();

    debugTask->startTask();
}

void CutterCore::toggleBreakpoint(RVA addr)
{
    CORE_LOCK();
    rz_core_debug_breakpoint_toggle(core, addr);
    emit breakpointsChanged(addr);
}

void CutterCore::addBreakpoint(const BreakpointDescription &config)
{
    CORE_LOCK();
    RzBreakpointItem *breakpoint = nullptr;
    int watchpoint_prot = 0;
    if (config.hw) {
        watchpoint_prot = config.permission & ~(RZ_PERM_X);
    }

    auto address = config.addr;
    char *module = nullptr;
    QByteArray moduleNameData;
    if (config.type == BreakpointDescription::Named) {
        address = Core()->math(config.positionExpression);
    } else if (config.type == BreakpointDescription::Module) {
        address = 0;
        moduleNameData = config.positionExpression.toUtf8();
        module = moduleNameData.data();
    }
    breakpoint = rz_debug_bp_add(core->dbg, address, config.size, config.hw, (watchpoint_prot != 0),
                                 watchpoint_prot, module, config.moduleDelta);
    if (!breakpoint) {
        QMessageBox::critical(nullptr, tr("Breakpoint error"), tr("Failed to create breakpoint"));
        return;
    }
    if (config.type == BreakpointDescription::Named) {
        updateOwnedCharPtr(breakpoint->expr, config.positionExpression);
    }

    if (config.hw) {
        breakpoint->size = config.size;
    }
    if (config.type == BreakpointDescription::Named) {
        updateOwnedCharPtr(breakpoint->name, config.positionExpression);
    }

    int index = std::find(core->dbg->bp->bps_idx,
                          core->dbg->bp->bps_idx + core->dbg->bp->bps_idx_count, breakpoint)
            - core->dbg->bp->bps_idx;

    breakpoint->enabled = config.enabled;
    if (config.trace) {
        setBreakpointTrace(index, config.trace);
    }
    if (!config.condition.isEmpty()) {
        updateOwnedCharPtr(breakpoint->cond, config.condition);
    }
    if (!config.command.isEmpty()) {
        updateOwnedCharPtr(breakpoint->data, config.command);
    }
    emit breakpointsChanged(breakpoint->addr);
}

void CutterCore::updateBreakpoint(int index, const BreakpointDescription &config)
{
    CORE_LOCK();
    if (auto bp = rz_bp_get_index(core->dbg->bp, index)) {
        rz_bp_del(core->dbg->bp, bp->addr);
    }
    // Delete by index currently buggy,
    // required for breakpoints with non address based position
    // rz_bp_del_index(core->dbg->bp, index);
    addBreakpoint(config);
}

void CutterCore::delBreakpoint(RVA addr)
{
    CORE_LOCK();
    rz_bp_del(core->dbg->bp, addr);
    emit breakpointsChanged(addr);
}

void CutterCore::delAllBreakpoints()
{
    CORE_LOCK();
    rz_bp_del_all(core->dbg->bp);
    emit refreshCodeViews();
}

void CutterCore::enableBreakpoint(RVA addr)
{
    CORE_LOCK();
    rz_bp_enable(core->dbg->bp, addr, true, 1);
    emit breakpointsChanged(addr);
}

void CutterCore::disableBreakpoint(RVA addr)
{
    CORE_LOCK();
    rz_bp_enable(core->dbg->bp, addr, false, 1);
    emit breakpointsChanged(addr);
}

void CutterCore::setBreakpointTrace(int index, bool enabled)
{
    CORE_LOCK();
    RzBreakpointItem *bpi = rz_bp_get_index(core->dbg->bp, index);
    bpi->trace = enabled;
}

static BreakpointDescription breakpointDescriptionFromRizin(int index, rz_bp_item_t *bpi)
{
    BreakpointDescription bp;
    bp.addr = bpi->addr;
    bp.index = index;
    bp.size = bpi->size;
    if (bpi->expr) {
        bp.positionExpression = bpi->expr;
        bp.type = BreakpointDescription::Named;
    }
    bp.name = bpi->name;
    bp.permission = bpi->perm;
    bp.command = bpi->data;
    bp.condition = bpi->cond;
    bp.hw = bpi->hw;
    bp.trace = bpi->trace;
    bp.enabled = bpi->enabled;
    return bp;
}

int CutterCore::breakpointIndexAt(RVA addr)
{
    CORE_LOCK();
    return rz_bp_get_index_at(core->dbg->bp, addr);
}

BreakpointDescription CutterCore::getBreakpointAt(RVA addr)
{
    CORE_LOCK();
    int index = rz_bp_get_index_at(core->dbg->bp, addr);
    auto bp = rz_bp_get_index(core->dbg->bp, index);
    if (bp) {
        return breakpointDescriptionFromRizin(index, bp);
    }
    return BreakpointDescription();
}

QList<BreakpointDescription> CutterCore::getBreakpoints()
{
    CORE_LOCK();
    QList<BreakpointDescription> ret;
    // TODO: use higher level API, don't touch rizin bps_idx directly
    for (int i = 0; i < core->dbg->bp->bps_idx_count; i++) {
        if (auto bpi = core->dbg->bp->bps_idx[i]) {
            ret.push_back(breakpointDescriptionFromRizin(i, bpi));
        }
    }

    return ret;
}

QList<RVA> CutterCore::getBreakpointsAddresses()
{
    QList<RVA> bpAddresses;
    for (const BreakpointDescription &bp : getBreakpoints()) {
        bpAddresses << bp.addr;
    }

    return bpAddresses;
}

QList<RVA> CutterCore::getBreakpointsInFunction(RVA funcAddr)
{
    QList<RVA> allBreakpoints = getBreakpointsAddresses();
    QList<RVA> functionBreakpoints;

    // Use std manipulations to take only the breakpoints that belong to this function
    std::copy_if(allBreakpoints.begin(), allBreakpoints.end(),
                 std::back_inserter(functionBreakpoints),
                 [this, funcAddr](RVA BPadd) { return getFunctionStart(BPadd) == funcAddr; });
    return functionBreakpoints;
}

bool CutterCore::isBreakpoint(const QList<RVA> &breakpoints, RVA addr)
{
    return breakpoints.contains(addr);
}

QList<ThreadDescription> CutterCore::getProcessThreads(int pid)
{
    CORE_LOCK();
    auto dbg = core_->dbg;
    if (!dbg || !dbg->cur || !dbg->cur->threads) {
        return {};
    }
    RzList *list = core_->dbg->cur->threads(dbg, pid != -1 ? pid : dbg->pid);
    RzListIter *iter;
    RzDebugPid *p;
    QList<ThreadDescription> ret;

    CutterRzListForeach (list, iter, RzDebugPid, p) {
        ThreadDescription proc;

        proc.current = dbg->tid == p->pid;
        proc.ppid = p->ppid;
        proc.pid = p->pid;
        proc.uid = p->uid;
        proc.status = static_cast<RzDebugPidState>(p->status);
        proc.path = p->path;
        proc.pc = p->pc;
        proc.tls = p->tls;

        ret << proc;
    }
    rz_list_free(list);
    return ret;
}

QList<ProcessDescription> CutterCore::getProcesses(int pid)
{
    CORE_LOCK();
    auto dbg = core_->dbg;
    if (!dbg || !dbg->cur || !dbg->cur->threads) {
        return {};
    }
    RzList *list = core_->dbg->cur->pids(dbg, pid >= 0 ? pid : dbg->pid);
    RzListIter *iter;
    RzDebugPid *p;
    QList<ProcessDescription> ret;

    CutterRzListForeach (list, iter, RzDebugPid, p) {
        ProcessDescription proc;

        proc.current = core->dbg->pid == p->pid;
        proc.ppid = p->ppid;
        proc.pid = p->pid;
        proc.uid = p->uid;
        proc.status = static_cast<RzDebugPidState>(p->status);
        proc.path = p->path;

        ret << proc;
    }
    rz_list_free(list);
    return ret;
}

QList<MemoryMapDescription> CutterCore::getMemoryMap()
{
    CORE_LOCK();
    RzList *list0 = rz_debug_map_list(core->dbg, false);
    RzList *list1 = rz_debug_map_list(core->dbg, true);
    rz_list_join(list0, list1);
    QList<MemoryMapDescription> ret;
    RzListIter *it;
    RzDebugMap *map;
    CutterRzListForeach (list0, it, RzDebugMap, map) {
        MemoryMapDescription memMap;

        memMap.name = map->name;
        memMap.fileName = map->file;
        memMap.addrStart = map->addr;
        memMap.addrEnd = map->addr_end;
        memMap.type = map->user ? "u" : "s";
        memMap.permission = rz_str_rwx_i(map->perm);

        ret << memMap;
    }

    return ret;
}

void CutterCore::setGraphEmpty(bool empty)
{
    emptyGraph = empty;
}

bool CutterCore::isGraphEmpty()
{
    return emptyGraph;
}

void CutterCore::getRegs()
{
    CORE_LOCK();
    this->regs = {};
    const RzList *rs = rz_reg_get_list(getReg(), RZ_REG_TYPE_ANY);
    if (!rs) {
        return;
    }
    for (const auto &r : CutterRzList<RzRegItem>(rs)) {
        this->regs.push_back(r->name);
    }
}

void CutterCore::setSettings()
{
    setConfig("scr.interactive", false);

    setConfig("hex.pairs", false);
    setConfig("asm.xrefs", false);

    setConfig("asm.tabs.once", true);
    setConfig("asm.flags.middle", 2);

    setConfig("analysis.hasnext", false);
    setConfig("asm.lines.call", false);

    // Colors
    setConfig("scr.color", COLOR_MODE_DISABLED);

    // Don't show hits
    setConfig("search.flags", false);
}

QList<RVA> CutterCore::getSeekHistory()
{
    CORE_LOCK();
    QList<RVA> ret;
    RzListIter *it;
    RzCoreSeekItem *undo;
    RzList *list = rz_core_seek_list(core);
    CutterRzListForeach (list, it, RzCoreSeekItem, undo) {
        ret << undo->offset;
    }

    return ret;
}

QStringList CutterCore::getAsmPluginNames()
{
    CORE_LOCK();
    QStringList ret;
    CutterHtSP<RzAsmPlugin>(core->rasm->plugins)
            .ForEach([&ret](const char *k, const RzAsmPlugin *ap) {
                ret << ap->name;
                return true;
            });
    return ret;
}

QStringList CutterCore::getAnalysisPluginNames()
{
    CORE_LOCK();
    RzListIter *it;
    QStringList ret;
    CutterHtSP<RzAnalysisPlugin>(core->analysis->plugins)
            .ForEach([&ret](const char *k, const RzAnalysisPlugin *ap) {
                ret << ap->name;
                return true;
            });
    return ret;
}

QList<RzBinPluginDescription> CutterCore::getBinPluginDescriptions(bool bin, bool xtr)
{
    CORE_LOCK();
    QList<RzBinPluginDescription> ret;
    if (bin) {
        CutterHtSP<RzBinPlugin>(core->bin->plugins)
                .ForEach([&ret](const char *k, const RzBinPlugin *bp) {
                    RzBinPluginDescription desc;
                    desc.name = bp->name ? bp->name : "";
                    desc.description = bp->desc ? bp->desc : "";
                    desc.license = bp->license ? bp->license : "";
                    desc.type = "bin";
                    ret.append(desc);
                    return true;
                });
    }
    if (xtr) {
        CutterHtSP<RzBinXtrPlugin>(core->bin->binxtrs)
                .ForEach([&ret](const char *k, const RzBinXtrPlugin *bx) {
                    RzBinPluginDescription desc;
                    desc.name = bx->name ? bx->name : "";
                    desc.description = bx->desc ? bx->desc : "";
                    desc.license = bx->license ? bx->license : "";
                    desc.type = "xtr";
                    ret.append(desc);
                    return true;
                });
    }
    return ret;
}

QList<RzIOPluginDescription> CutterCore::getRIOPluginDescriptions()
{
    CORE_LOCK();
    QList<RzIOPluginDescription> ret;
    CutterHtSP<RzIOPlugin>(core->io->plugins).ForEach([&ret](const char *k, const RzIOPlugin *p) {
        RzIOPluginDescription desc;
        desc.name = p->name ? p->name : "";
        desc.description = p->desc ? p->desc : "";
        desc.license = p->license ? p->license : "";
        desc.permissions = QString("r") + (p->write ? "w" : "_") + (p->isdbg ? "d" : "_");
        if (p->uris) {
            desc.uris = QString::fromUtf8(p->uris).split(",");
        }
        ret.append(desc);
        return true;
    });
    return ret;
}

QList<RzCorePluginDescription> CutterCore::getRCorePluginDescriptions()
{
    CORE_LOCK();
    QList<RzCorePluginDescription> ret;
    CutterHtSP<RzCorePlugin>(core->plugins).ForEach([&ret](const char *k, const RzCorePlugin *p) {
        RzCorePluginDescription desc;
        desc.name = p->name ? p->name : "";
        desc.description = p->desc ? p->desc : "";
        desc.license = p->license ? p->license : "";
        ret.append(desc);
        return true;
    });
    return ret;
}

QList<RzAsmPluginDescription> CutterCore::getRAsmPluginDescriptions()
{
    CORE_LOCK();
    QList<RzAsmPluginDescription> ret;

    CutterHtSP<RzAsmPlugin>(core->rasm->plugins)
            .ForEach([&ret](const char *k, const RzAsmPlugin *ap) {
                RzAsmPluginDescription plugin;

                plugin.name = ap->name;
                plugin.architecture = ap->arch;
                plugin.author = ap->author;
                plugin.version = ap->version;
                plugin.cpus = ap->cpus;
                plugin.description = ap->desc;
                plugin.license = ap->license;

                ret << plugin;
                return true;
            });

    return ret;
}

QList<FunctionDescription> CutterCore::getAllFunctions()
{
    CORE_LOCK();

    QList<FunctionDescription> funcList;
    funcList.reserve(rz_list_length(core->analysis->fcns));

    RzListIter *iter;
    RzAnalysisFunction *fcn;
    CutterRzListForeach (core->analysis->fcns, iter, RzAnalysisFunction, fcn) {
        FunctionDescription function;
        function.offset = fcn->addr;
        function.linearSize = rz_analysis_function_linear_size(fcn);
        function.nargs = rz_analysis_arg_count(fcn);
        function.nlocals = rz_analysis_var_local_count(fcn);
        function.nbbs = rz_pvector_len(fcn->bbs);
        function.calltype = fcn->cc ? QString::fromUtf8(fcn->cc) : QString();
        function.name = fcn->name ? QString::fromUtf8(fcn->name) : QString();
        function.edges = rz_analysis_function_count_edges(fcn, nullptr);
        function.stackframe = fcn->maxstack;
        funcList.append(function);
    }

    return funcList;
}

static inline uint64_t rva(RzBinObject *o, uint64_t paddr, uint64_t vaddr, int va)
{
    return va ? rz_bin_object_get_vaddr(o, paddr, vaddr) : paddr;
}

QList<ImportDescription> CutterCore::getAllImports()
{
    CORE_LOCK();
    RzBinFile *bf = rz_bin_cur(core->bin);
    if (!bf) {
        return {};
    }
    const RzPVector *imports = rz_bin_object_get_imports(bf->o);
    if (!imports) {
        return {};
    }

    QList<ImportDescription> qList;
    bool va = core->io->va || core->bin->is_debugger;
    for (const auto &import : CutterPVector<RzBinImport>(imports)) {
        if (RZ_STR_ISEMPTY(import->name)) {
            continue;
        }

        ImportDescription importDescription;

        RzBinSymbol *sym = rz_bin_object_get_symbol_of_import(bf->o, import);
        ut64 addr = sym ? rva(bf->o, sym->paddr, sym->vaddr, va) : UT64_MAX;
        QString name { import->name };
        if (RZ_STR_ISNOTEMPTY(import->classname)) {
            name = QString("%1.%2").arg(import->classname, import->name);
        }
        if (core->bin->prefix) {
            name = QString("%1.%2").arg(core->bin->prefix, name);
        }

        importDescription.ordinal = (int)import->ordinal;
        importDescription.bind = import->bind;
        importDescription.type = import->type;
        importDescription.libname = import->libname;
        importDescription.name = name;
        importDescription.plt = addr;

        qList << importDescription;
    }

    return qList;
}

QList<ExportDescription> CutterCore::getAllExports()
{
    CORE_LOCK();
    RzBinFile *bf = rz_bin_cur(core->bin);
    if (!bf) {
        return {};
    }

    const RzPVector *symbols = rz_bin_object_get_symbols(bf->o);
    if (!symbols) {
        return {};
    }

    bool va = core->io->va || core->bin->is_debugger;
    bool demangle = rz_config_get_b(core->config, "bin.demangle");

    QList<ExportDescription> ret;
    for (const auto &symbol : CutterPVector<RzBinSymbol>(symbols)) {
        if (!(symbol->name && rz_core_sym_is_export(symbol))) {
            continue;
        }

        RzBinSymNames sn = {};
        rz_core_sym_name_init(&sn, symbol, demangle);

        ExportDescription exportDescription;
        exportDescription.vaddr = rva(bf->o, symbol->paddr, symbol->vaddr, va);
        exportDescription.paddr = symbol->paddr;
        exportDescription.size = symbol->size;
        exportDescription.type = symbol->type;
        exportDescription.name = sn.symbolname;
        exportDescription.flag_name = sn.nameflag;
        ret << exportDescription;

        rz_core_sym_name_fini(&sn);
    }
    return ret;
}

QList<SymbolDescription> CutterCore::getAllSymbols()
{
    CORE_LOCK();
    RzBinFile *bf = rz_bin_cur(core->bin);
    if (!bf) {
        return {};
    }

    QList<SymbolDescription> ret;
    const RzPVector *symbols = rz_bin_object_get_symbols(bf->o);
    if (symbols) {
        for (const auto &bs : CutterPVector<RzBinSymbol>(symbols)) {
            QString type = QString(bs->bind) + " " + QString(bs->type);
            SymbolDescription symbol;
            symbol.vaddr = bs->vaddr;
            symbol.name = QString(bs->name);
            symbol.bind = QString(bs->bind);
            symbol.type = QString(bs->type);
            ret << symbol;
        }
    }

    const RzPVector *entries = rz_bin_object_get_entries(bf->o);
    if (symbols) {
        /* list entrypoints as symbols too */
        int n = 0;
        for (const auto &entry : CutterPVector<RzBinAddr>(entries)) {
            SymbolDescription symbol;
            symbol.vaddr = entry->vaddr;
            symbol.name = QString("entry") + QString::number(n++);
            symbol.bind.clear();
            symbol.type = "entry";
            ret << symbol;
        }
    }

    return ret;
}

QList<HeaderDescription> CutterCore::getAllHeaders()
{
    CORE_LOCK();
    RzBinFile *bf = rz_bin_cur(core->bin);
    if (!bf) {
        return {};
    }
    const RzPVector *fields = rz_bin_object_get_fields(bf->o);
    if (!fields) {
        return {};
    }
    QList<HeaderDescription> ret;
    for (auto field : CutterPVector<RzBinField>(fields)) {
        HeaderDescription header;
        header.vaddr = field->vaddr;
        header.paddr = field->paddr;
        header.value = RZ_STR_ISEMPTY(field->comment) ? "" : field->comment;
        header.name = field->name;
        ret << header;
    }
    return ret;
}

QList<FlirtDescription> CutterCore::getSignaturesDB()
{
    CORE_LOCK();

    void *ptr = NULL;
    RzListIter *iter = NULL;
    QList<FlirtDescription> sigdb;

    RzList *list = rz_core_analysis_sigdb_list(core, true);

    rz_list_foreach(list, iter, ptr)
    {
        RzSigDBEntry *sig = static_cast<RzSigDBEntry *>(ptr);
        FlirtDescription flirt;
        flirt.bin_name = sig->bin_name;
        flirt.arch_name = sig->arch_name;
        flirt.base_name = sig->base_name;
        flirt.short_path = sig->short_path;
        flirt.file_path = sig->file_path;
        flirt.details = sig->details;
        flirt.n_modules = QString::number(sig->n_modules);
        flirt.arch_bits = QString::number(sig->arch_bits);
        sigdb << flirt;
    }
    rz_list_free(list);

    return sigdb;
}

QList<CommentDescription> CutterCore::getAllComments(const QString &filterType)
{
    CORE_LOCK();
    QList<CommentDescription> qList;
    RzIntervalTreeIter it;
    void *pVoid;
    RzAnalysisMetaItem *item;
    RzSpace *spaces = rz_spaces_current(&core->analysis->meta_spaces);
    rz_interval_tree_foreach(&core->analysis->meta, it, pVoid)
    {
        item = reinterpret_cast<RzAnalysisMetaItem *>(pVoid);
        if (item->type != RZ_META_TYPE_COMMENT) {
            continue;
        }
        if (spaces && spaces != item->space) {
            continue;
        }
        if (filterType != rz_meta_type_to_string(item->type)) {
            continue;
        }

        RzIntervalNode *node = rz_interval_tree_iter_get(&it);
        CommentDescription comment;
        comment.offset = node->start;
        comment.name = fromOwnedCharPtr(rz_str_escape(item->str));
        qList << comment;
    }
    return qList;
}

QList<RelocDescription> CutterCore::getAllRelocs()
{
    CORE_LOCK();
    QList<RelocDescription> ret;

    if (core && core->bin && core->bin->cur && core->bin->cur->o) {
        auto relocs = rz_bin_object_patch_relocs(core->bin->cur, core->bin->cur->o);
        if (!relocs) {
            return ret;
        }
        for (size_t i = 0; i < relocs->relocs_count; i++) {
            RzBinReloc *reloc = relocs->relocs[i];
            RelocDescription desc;
            desc.vaddr = reloc->vaddr;
            desc.paddr = reloc->paddr;
            desc.type = (reloc->additive ? "ADD_" : "SET_") + QString::number(reloc->type);

            if (reloc->import)
                desc.name = reloc->import->name;
            else
                desc.name = QString("reloc_%1").arg(QString::number(reloc->vaddr, 16));

            ret << desc;
        }
    }

    return ret;
}

QList<StringDescription> CutterCore::getAllStrings()
{
    CORE_LOCK();
    RzBinFile *bf = rz_bin_cur(core->bin);
    if (!bf) {
        return {};
    }
    RzBinObject *obj = rz_bin_cur_object(core->bin);
    if (!obj) {
        return {};
    }

    RzPVector *strings = rz_core_bin_whole_strings(core, bf);
    if (!strings) {
        return {};
    }

    int va = core->io->va || core->bin->is_debugger;
    RzStrEscOptions opt = {};
    opt.show_asciidot = false;
    opt.esc_bslash = true;
    opt.esc_double_quotes = true;
    opt.keep_printable = true;

    QList<StringDescription> ret;
    for (const auto &str : CutterPVector<RzBinString>(strings)) {
        auto section = obj ? rz_bin_get_section_at(obj, str->paddr, 0) : NULL;

        StringDescription string;
        string.string = rz_str_escape_utf8(str->string, &opt);
        string.vaddr = obj ? rva(obj, str->paddr, str->vaddr, va) : str->paddr;
        string.type = rz_str_enc_as_string(str->type);
        string.size = str->size;
        string.length = str->length;
        string.section = section ? section->name : "";

        ret << string;
    }

    return ret;
}

QList<FlagspaceDescription> CutterCore::getAllFlagspaces()
{
    CORE_LOCK();
    QList<FlagspaceDescription> flagspaces;
    RzSpaceIter it;
    RzSpace *space;
    rz_flag_space_foreach(core->flags, it, space)
    {
        FlagspaceDescription flagspace;
        flagspace.name = space->name;
        flagspaces << flagspace;
    }
    return flagspaces;
}

QList<FlagDescription> CutterCore::getAllFlags(QString flagspace)
{
    CORE_LOCK();
    QList<FlagDescription> flags;
    std::string name = flagspace.isEmpty() || flagspace.isNull() ? "*" : flagspace.toStdString();
    RzSpace *space = rz_flag_space_get(core->flags, name.c_str());
    rz_flag_foreach_space(
            core->flags, space,
            [](RzFlagItem *item, void *user) {
                FlagDescription flag;
                flag.offset = item->offset;
                flag.size = item->size;
                flag.name = item->name;
                flag.realname = item->name;
                reinterpret_cast<QList<FlagDescription> *>(user)->append(flag);
                return true;
            },
            &flags);
    return flags;
}

QList<SectionDescription> CutterCore::getAllSections()
{
    CORE_LOCK();
    QList<SectionDescription> sections;

    RzBinObject *o = rz_bin_cur_object(core->bin);
    if (!o) {
        return sections;
    }

    RzPVector *sects = rz_bin_object_get_sections(o);
    if (!sects) {
        return sections;
    }
    RzList *hashnames = rz_list_newf(free);
    if (!hashnames) {
        return sections;
    }
    rz_list_push(hashnames, rz_str_dup("entropy"));
    for (const auto &sect : CutterPVector<RzBinSection>(sects)) {
        if (RZ_STR_ISEMPTY(sect->name))
            continue;

        SectionDescription section;
        section.name = sect->name;
        section.vaddr = sect->vaddr;
        section.vsize = sect->vsize;
        section.paddr = sect->paddr;
        section.size = sect->size;
        section.perm = rz_str_rwx_i(sect->perm);
        if (sect->size > 0) {
            HtSS *digests = rz_core_bin_create_digests(core, sect->paddr, sect->size, hashnames);
            if (!digests) {
                continue;
            }
            const char *entropy = (const char *)ht_ss_find(digests, "entropy", NULL);
            section.entropy = rz_str_get(entropy);
            ht_ss_free(digests);
        }

        sections << section;
    }
    rz_pvector_free(sects);
    return sections;
}

QStringList CutterCore::getSectionList()
{
    CORE_LOCK();
    QStringList ret;

    RzBinObject *o = rz_bin_cur_object(core->bin);
    if (!o) {
        return ret;
    }

    RzPVector *sects = rz_bin_object_get_sections(o);
    if (!sects) {
        return ret;
    }
    for (const auto &sect : CutterPVector<RzBinSection>(sects)) {
        ret << sect->name;
    }
    rz_pvector_free(sects);
    return ret;
}

static inline QString perms_str(int perms)
{
    return QString((perms & RZ_PERM_SHAR) ? 's' : '-') + rz_str_rwx_i(perms);
}

QList<SegmentDescription> CutterCore::getAllSegments()
{
    CORE_LOCK();
    RzBinFile *bf = rz_bin_cur(core->bin);
    if (!bf) {
        return {};
    }
    RzPVector *segments = rz_bin_object_get_segments(bf->o);
    if (!segments) {
        return {};
    }

    QList<SegmentDescription> ret;
    for (const auto &segment : CutterPVector<RzBinSection>(segments)) {
        SegmentDescription segDesc;
        segDesc.name = segment->name;
        segDesc.vaddr = segment->vaddr;
        segDesc.paddr = segment->paddr;
        segDesc.size = segment->size;
        segDesc.vsize = segment->vsize;
        segDesc.perm = perms_str(segment->perm);
        ret << segDesc;
    }
    rz_pvector_free(segments);

    return ret;
}

QList<EntrypointDescription> CutterCore::getAllEntrypoint()
{
    CORE_LOCK();
    RzBinFile *bf = rz_bin_cur(core->bin);
    if (!bf) {
        return {};
    }
    bool va = core->io->va || core->bin->is_debugger;
    ut64 baddr = rz_bin_get_baddr(core->bin);
    ut64 laddr = rz_bin_get_laddr(core->bin);

    QList<EntrypointDescription> qList;
    const RzPVector *entries = rz_bin_object_get_entries(bf->o);
    for (const auto &entry : CutterPVector<RzBinAddr>(entries)) {
        if (entry->type != RZ_BIN_ENTRY_TYPE_PROGRAM) {
            continue;
        }
        const char *type = rz_bin_entry_type_string(entry->type);

        EntrypointDescription entrypointDescription;
        entrypointDescription.vaddr = rva(bf->o, entry->paddr, entry->vaddr, va);
        entrypointDescription.paddr = entry->paddr;
        entrypointDescription.baddr = baddr;
        entrypointDescription.laddr = laddr;
        entrypointDescription.haddr = entry->hpaddr ? entry->hpaddr : UT64_MAX;
        entrypointDescription.type = type ? type : "unknown";
        qList << entrypointDescription;
    }

    return qList;
}

QList<BinClassDescription> CutterCore::getAllClassesFromBin()
{
    CORE_LOCK();
    RzBinFile *bf = rz_bin_cur(core->bin);
    if (!bf) {
        return {};
    }

    const RzPVector *cs = rz_bin_object_get_classes(bf->o);
    if (!cs) {
        return {};
    }

    QList<BinClassDescription> qList;
    RzListIter *iter2, *iter3;
    RzBinSymbol *sym;
    RzBinClassField *f;
    for (const auto &c : CutterPVector<RzBinClass>(cs)) {
        BinClassDescription classDescription;
        classDescription.name = c->name;
        classDescription.addr = c->addr;
        CutterRzListForeach (c->methods, iter2, RzBinSymbol, sym) {
            BinClassMethodDescription methodDescription;
            methodDescription.name = sym->name;
            methodDescription.addr = sym->vaddr;
            classDescription.methods << methodDescription;
        }
        CutterRzListForeach (c->fields, iter3, RzBinClassField, f) {
            BinClassFieldDescription fieldDescription;
            fieldDescription.name = f->name;
            fieldDescription.addr = f->vaddr;
            classDescription.fields << fieldDescription;
        }
        qList << classDescription;
    }
    return qList;
}

QList<BinClassDescription> CutterCore::getAllClassesFromFlags()
{
    static const QRegularExpression classFlagRegExp("^class\\.(.*)$");
    static const QRegularExpression methodFlagRegExp("^method\\.([^\\.]*)\\.(.*)$");

    CORE_LOCK();
    QList<BinClassDescription> ret;
    QMap<QString, BinClassDescription *> classesCache;

    for (const auto &item : getAllFlags("classes")) {
        QRegularExpressionMatch match = classFlagRegExp.match(item.name);
        if (match.hasMatch()) {
            QString className = match.captured(1);
            BinClassDescription *desc = nullptr;
            auto it = classesCache.find(className);
            if (it == classesCache.end()) {
                BinClassDescription cls = {};
                ret << cls;
                desc = &ret.last();
                classesCache[className] = desc;
            } else {
                desc = it.value();
            }
            desc->name = match.captured(1);
            desc->addr = item.offset;
            continue;
        }

        match = methodFlagRegExp.match(item.name);
        if (match.hasMatch()) {
            QString className = match.captured(1);
            BinClassDescription *classDesc = nullptr;
            auto it = classesCache.find(className);
            if (it == classesCache.end()) {
                // add a new stub class, will be replaced if class flag comes after it
                BinClassDescription cls;
                cls.name = tr("Unknown (%1)").arg(className);
                cls.addr = RVA_INVALID;
                ret << cls;
                classDesc = &ret.last();
                classesCache[className] = classDesc;
            } else {
                classDesc = it.value();
            }

            BinClassMethodDescription meth;
            meth.name = match.captured(2);
            meth.addr = item.offset;
            classDesc->methods << meth;
            continue;
        }
    }
    return ret;
}

QList<QString> CutterCore::getAllAnalysisClasses(bool sorted)
{
    CORE_LOCK();
    QList<QString> ret;
    PVectorPtr l = makePVectorPtr(rz_analysis_class_get_all(core->analysis, sorted));
    if (!l) {
        return ret;
    }
    ret.reserve(static_cast<int>(rz_pvector_len(l.get())));
    for (const auto &kv : CutterPVector<SdbKv>(l.get())) {
        ret.append(QString::fromUtf8(reinterpret_cast<const char *>(kv->base.key)));
    }
    return ret;
}

QList<AnalysisMethodDescription> CutterCore::getAnalysisClassMethods(const QString &cls)
{
    CORE_LOCK();
    QList<AnalysisMethodDescription> ret;

    RzVector *meths = rz_analysis_class_method_get_all(core->analysis, cls.toUtf8().constData());
    if (!meths) {
        return ret;
    }

    ret.reserve(static_cast<int>(meths->len));
    RzAnalysisMethod *meth;
    CutterRzVectorForeach(meths, meth, RzAnalysisMethod)
    {
        AnalysisMethodDescription desc;
        desc.name = QString::fromUtf8(meth->name);
        desc.realName = QString::fromUtf8(meth->real_name);
        desc.addr = meth->addr;
        desc.vtableOffset = meth->vtable_offset;
        ret.append(desc);
    }
    rz_vector_free(meths);

    return ret;
}

QList<AnalysisBaseClassDescription> CutterCore::getAnalysisClassBaseClasses(const QString &cls)
{
    CORE_LOCK();
    QList<AnalysisBaseClassDescription> ret;

    RzVector *bases = rz_analysis_class_base_get_all(core->analysis, cls.toUtf8().constData());
    if (!bases) {
        return ret;
    }

    ret.reserve(static_cast<int>(bases->len));
    RzAnalysisBaseClass *base;
    CutterRzVectorForeach(bases, base, RzAnalysisBaseClass)
    {
        AnalysisBaseClassDescription desc;
        desc.id = QString::fromUtf8(base->id);
        desc.offset = base->offset;
        desc.className = QString::fromUtf8(base->class_name);
        ret.append(desc);
    }
    rz_vector_free(bases);

    return ret;
}

QList<AnalysisVTableDescription> CutterCore::getAnalysisClassVTables(const QString &cls)
{
    CORE_LOCK();
    QList<AnalysisVTableDescription> acVtables;

    RzVector *vtables = rz_analysis_class_vtable_get_all(core->analysis, cls.toUtf8().constData());
    if (!vtables) {
        return acVtables;
    }

    acVtables.reserve(static_cast<int>(vtables->len));
    RzAnalysisVTable *vtable;
    CutterRzVectorForeach(vtables, vtable, RzAnalysisVTable)
    {
        AnalysisVTableDescription desc;
        desc.id = QString::fromUtf8(vtable->id);
        desc.offset = vtable->offset;
        desc.addr = vtable->addr;
        acVtables.append(desc);
    }
    rz_vector_free(vtables);

    return acVtables;
}

void CutterCore::createNewClass(const QString &cls)
{
    CORE_LOCK();
    rz_analysis_class_create(core->analysis, cls.toUtf8().constData());
}

void CutterCore::renameClass(const QString &oldName, const QString &newName)
{
    CORE_LOCK();
    rz_analysis_class_rename(core->analysis, oldName.toUtf8().constData(),
                             newName.toUtf8().constData());
}

void CutterCore::deleteClass(const QString &cls)
{
    CORE_LOCK();
    rz_analysis_class_delete(core->analysis, cls.toUtf8().constData());
}

bool CutterCore::getAnalysisMethod(const QString &cls, const QString &meth,
                                   AnalysisMethodDescription *desc)
{
    CORE_LOCK();
    RzAnalysisMethod analysisMeth;
    if (rz_analysis_class_method_get(core->analysis, cls.toUtf8().constData(),
                                     meth.toUtf8().constData(), &analysisMeth)
        != RZ_ANALYSIS_CLASS_ERR_SUCCESS) {
        return false;
    }
    desc->name = QString::fromUtf8(analysisMeth.name);
    desc->realName = QString::fromUtf8(analysisMeth.real_name);
    desc->addr = analysisMeth.addr;
    desc->vtableOffset = analysisMeth.vtable_offset;
    rz_analysis_class_method_fini(&analysisMeth);
    return true;
}

void CutterCore::setAnalysisMethod(const QString &className, const AnalysisMethodDescription &meth)
{
    CORE_LOCK();
    RzAnalysisMethod analysisMeth;
    analysisMeth.name = rz_str_dup(meth.name.toUtf8().constData());
    analysisMeth.real_name = rz_str_dup(meth.realName.toUtf8().constData());
    analysisMeth.addr = meth.addr;
    analysisMeth.vtable_offset = meth.vtableOffset;
    rz_analysis_class_method_set(core->analysis, className.toUtf8().constData(), &analysisMeth);
    rz_analysis_class_method_fini(&analysisMeth);
}

void CutterCore::renameAnalysisMethod(const QString &className, const QString &oldMethodName,
                                      const QString &newMethodName)
{
    CORE_LOCK();
    rz_analysis_class_method_rename(core->analysis, className.toUtf8().constData(),
                                    oldMethodName.toUtf8().constData(),
                                    newMethodName.toUtf8().constData());
}

QList<ResourcesDescription> CutterCore::getAllResources()
{
    CORE_LOCK();
    RzBinFile *bf = rz_bin_cur(core->bin);
    if (!bf) {
        return {};
    }
    const RzPVector *resources = rz_bin_object_get_resources(bf->o);
    if (!resources) {
        return {};
    }

    QList<ResourcesDescription> resourcesDescriptions;

    for (const auto &resource : CutterPVector<RzBinResource>(resources)) {
        ResourcesDescription description;
        description.name = resource->name;
        description.vaddr = resource->vaddr;
        description.index = resource->index;
        description.type = resource->type;
        description.size = resource->size;
        description.lang = resource->language;

        resourcesDescriptions << description;
    }

    return resourcesDescriptions;
}

QList<VTableDescription> CutterCore::getAllVTables()
{
    CORE_LOCK();
    QList<VTableDescription> vtableDescs;
    RVTableContext context;
    rz_analysis_vtable_begin(core->analysis, &context);
    RzList *vtables = rz_analysis_vtable_search(&context);
    RzListIter *iter;
    RVTableInfo *table;
    RVTableMethodInfo *method;
    CutterRzListForeach (vtables, iter, RVTableInfo, table) {
        VTableDescription tableDesc;
        tableDesc.addr = table->saddr;
        CutterRzVectorForeach(&table->methods, method, RVTableMethodInfo)
        {
            BinClassMethodDescription methodDesc;
            RzAnalysisFunction *fcn = rz_analysis_get_fcn_in(core->analysis, method->addr, 0);
            const char *fname = fcn ? fcn->name : nullptr;
            methodDesc.addr = method->addr;
            methodDesc.name = fname;
            tableDesc.methods << methodDesc;
        }
        vtableDescs << tableDesc;
    }
    rz_list_free(vtables);
    return vtableDescs;
}

QList<TypeDescription> CutterCore::getAllTypes()
{
    QList<TypeDescription> types;

    types.append(getAllPrimitiveTypes());
    types.append(getAllUnions());
    types.append(getAllStructs());
    types.append(getAllEnums());
    types.append(getAllTypedefs());

    return types;
}

QList<TypeDescription> CutterCore::getBaseType(RzBaseTypeKind kind, const char *category)
{
    CORE_LOCK();
    QList<TypeDescription> types;

    RzList *ts = rz_type_db_get_base_types_of_kind(core->analysis->typedb, kind);
    RzBaseType *type;
    RzListIter *iter;

    CutterRzListForeach (ts, iter, RzBaseType, type) {
        TypeDescription exp;

        exp.type = type->name;
        exp.size = rz_type_db_base_get_bitsize(core->analysis->typedb, type);
        exp.format = rz_base_type_as_format(core->analysis->typedb, type);
        exp.category = tr(category);
        types << exp;
    }
    rz_list_free(ts);

    return types;
}

QList<TypeDescription> CutterCore::getAllPrimitiveTypes()
{
    return getBaseType(RZ_BASE_TYPE_KIND_ATOMIC, "Primitive");
}

QList<TypeDescription> CutterCore::getAllUnions()
{
    return getBaseType(RZ_BASE_TYPE_KIND_UNION, "Union");
}

QList<TypeDescription> CutterCore::getAllStructs()
{
    return getBaseType(RZ_BASE_TYPE_KIND_STRUCT, "Struct");
}

QList<TypeDescription> CutterCore::getAllEnums()
{
    return getBaseType(RZ_BASE_TYPE_KIND_ENUM, "Enum");
}

QList<TypeDescription> CutterCore::getAllTypedefs()
{
    return getBaseType(RZ_BASE_TYPE_KIND_TYPEDEF, "Typedef");
}

QString CutterCore::getTypeAsC(QString name)
{
    CORE_LOCK();
    QString output = "Failed to fetch the output.";
    if (name.isEmpty()) {
        return output;
    }
    char *earg = rz_cmd_escape_arg(name.toUtf8().constData(), RZ_CMD_ESCAPE_ONE_ARG);
    QString result = fromOwnedCharPtr(rz_core_types_as_c(core, earg, true));
    free(earg);
    return result;
}

bool CutterCore::isAddressMapped(RVA addr)
{
    CORE_LOCK();
    return rz_io_map_get(core->io, addr);
}

QList<SearchDescription> CutterCore::getAllSearchCommand(QString searchFor, SearchKind kind,
                                                         QString in)
{
    CORE_LOCK();
    QList<SearchDescription> searchRef;

    TempConfig cfg;
    cfg.set("search.in", in);
    CutterJson searchArray;
    char *arg = rz_cmd_escape_arg(searchFor.toUtf8().constData(), RZ_CMD_ESCAPE_ONE_ARG);
    if (!arg) {
        return {};
    }

    QString cmd, suffix;
    // Those are the searches which don't follow the search hit standardization of the new
    // search yet.
    switch (kind) {
    default:
        qWarning() << tr("Error invalid search kind\n");
        return searchRef;
    case SearchKind::AsmCode:
        cmd = "/acj";
        break;
    case SearchKind::ROPGadgets:
        cmd = "/Rj";
        break;
    case SearchKind::ROPGadgetsRegex:
        cmd = "/R/j";
        break;
    }
    // Legacy commands don't get escaped arguments.
    auto cstr = QString("%1 %2").arg(cmd, kind == SearchKind::AsmCode ? searchFor : arg);
    searchArray = cmdj(cstr);
    if (kind == SearchKind::ROPGadgets || kind == SearchKind::ROPGadgetsRegex) {
        for (CutterJson searchObject : searchArray) {
            SearchDescription exp;

            exp.code.clear();
            for (CutterJson gadget : searchObject[RJsonKey::opcodes]) {
                exp.code += gadget[RJsonKey::opcode].toString() + ";  ";
            }

            exp.offset = searchObject[RJsonKey::opcodes].first()[RJsonKey::offset].toRVA();
            exp.size = searchObject[RJsonKey::size].toUt64();

            searchRef << exp;
        }
        return searchRef;
    }
    for (CutterJson searchObject : searchArray) {
        SearchDescription exp;

        exp.offset = searchObject[RJsonKey::offset].toRVA();
        exp.size = searchObject[RJsonKey::len].toUt64();
        exp.code = searchObject[RJsonKey::code].toString();
        exp.data = searchObject[RJsonKey::data].toString();
        exp.detail = rz_meta_get_string(core->analysis, RZ_META_TYPE_COMMENT, exp.offset);

        searchRef << exp;
    }
    return searchRef;
}

static UniquePtrC<RzSearchOpt, &rz_search_opt_free> cutterSetupSearchOptions(RzCore *core)
{
    auto searchOpts = UniquePtrC<RzSearchOpt, &rz_search_opt_free>(rz_search_opt_new());
    if (!searchOpts) {
        return {};
    }
    RzThreadNCores max_threads =
            (RzThreadNCores)rz_config_get_i(core->config, "search.max_threads");
    max_threads = rz_th_max_threads(max_threads);
    ut32 max_hits = rz_config_get_i(core->config, "search.maxhits");
    const char *show_progress = rz_config_get(core->config, "search.show_progress");
    if (!(rz_search_opt_set_max_threads(searchOpts.get(), max_threads)
          && rz_search_opt_set_max_hits(searchOpts.get(), max_hits)
          && rz_search_opt_set_show_progress_from_str(searchOpts.get(), show_progress))) {
        RZ_LOG_ERROR("Failed setup find options.\n");
        return {};
    }

    RzSearchFindOpt *fopts = rz_core_setup_default_search_find_opts(core);
    if (!fopts) {
        RZ_LOG_ERROR("Failed init find options.\n");
        return {};
    }
    if (!rz_search_opt_set_find_options(searchOpts.get(), fopts)) {
        RZ_LOG_ERROR("Failed add find options to the search options.\n");
        return {};
    }
    return searchOpts;
}

class CutterSearchLock
{
public:
    CutterSearchLock(RzCore *core) : core_(core)
    {
        rz_cons_break_push(NULL, NULL);
        core_->in_search = true;
    }
    ~CutterSearchLock()
    {
        rz_cons_break_pop();
        core_->in_search = false;
    }

private:
    RzCore *core_ = nullptr;
};

static QString cutterGetSearchHitData(RzCore *core, SearchKind kind, RzSearchHit *hit)
{
    QString data = "";
    size_t dataSize = RZ_MAX(hit->size, 16);
    std::vector<ut8> buffer(dataSize);

    if (!rz_io_read_at(core->io, hit->address, buffer.data(), dataSize)) {
        return "";
    }

    switch (kind) {
    default:
        // for some kinds, we don't do anything.
        break;
    case SearchKind::Value32BE:
        /* fall-thru */
    case SearchKind::Value32LE:
        /* fall-thru */
    case SearchKind::Value64BE:
        /* fall-thru */
    case SearchKind::Value64LE:
        /* fall-thru */
    case SearchKind::HexString:
        /* fall-thru */
    case SearchKind::CryptographicMaterial:
        /* fall-thru */
    case SearchKind::MagicSignature: {
        data = fromOwnedCharPtr(rz_hex_bin2strdup(buffer.data(), dataSize));
        break;
    }
    case SearchKind::String:
        /* fall-thru */
    case SearchKind::StringCaseInsensitive:
        /* fall-thru */
    case SearchKind::StringRegexExtended: {
        RzStrStringifyOpt sopt;
        RzStrEnc encoding = RZ_STRING_ENC_GUESS;
        QString enc = hit->hit_desc;
        enc = enc.section(".", 1, 1);
        if (!enc.isEmpty()) {
            encoding = rz_str_enc_string_as_type(enc.toUtf8().constData());
        }

        if (encoding == RZ_STRING_ENC_GUESS) {
            encoding = rz_str_guess_encoding_from_buffer(buffer.data(), dataSize);
        }

        sopt.buffer = buffer.data();
        sopt.length = dataSize;
        sopt.encoding = encoding;
        sopt.wrap_at = 0;
        sopt.escape_nl = false;
        sopt.json = false;
        sopt.stop_at_nil = true;
        sopt.stop_at_unprintable = false;
        sopt.urlencode = false;

        data = fromOwnedCharPtr(rz_str_stringify_raw_buffer(&sopt, nullptr));
        break;
    }
    }

    return data;
}

static QString cutterValueAsHex(QString strVal, bool bigEndian, size_t size)
{
    ut64 value = rz_num_math(nullptr, strVal.toUtf8().constData());
    ut8 buffer[sizeof(ut64)] = { 0 };
    char output[64] = { 0 };
    rz_write_ble(buffer, value, bigEndian, size);
    rz_hex_bin2str(buffer, size == 32 ? 4 : 8, output);
    return output;
}

QList<SearchDescription> CutterCore::getAllSearch(QString searchFor, SearchKind kind, QString in)
{
    if (searchFor.isEmpty() && kind != SearchKind::CryptographicMaterial
        && kind != SearchKind::MagicSignature) {
        return {};
    }

    // call the oldsyle command for this search.
    // this must be done before CORE_LOCK() to avoid deadlocks.
    switch (kind) {
    case SearchKind::AsmCode:
        /* fall-thru */
    case SearchKind::ROPGadgets:
        /* fall-thru */
    case SearchKind::ROPGadgetsRegex:
        // old style search
        return getAllSearchCommand(searchFor, kind, in);
    default:
        // use C API
        break;
    }

    CORE_LOCK();

    if (core->in_search) {
        // this is impossible to happen, unless the user runs
        // search via terminal before calling the Qt UI
        RZ_LOG_ERROR("cutter: recursive search detected, search aborted.\n");
        return {};
    }

    // this takes ownership of the search lock
    CutterSearchLock searchLock(core);

    TempConfig cfg;
    cfg.set("search.in", in);

    QList<SearchDescription> searchRef;
    RzList /*<RzSearchHit *>*/ *hits = nullptr;
    auto userOpts = cutterSetupSearchOptions(core);
    if (!userOpts) {
        return searchRef;
    }

    switch (kind) {
    default:
        qWarning() << tr("Error invalid search kind\n");
        return searchRef;
    case SearchKind::HexString: {
        RzSearchBytesPattern *pattern =
                rz_search_parse_byte_pattern(searchFor.toUtf8().constData(), "bytes");
        if (!pattern) {
            return searchRef;
        }
        hits = rz_core_search_bytes(core, userOpts.get(), pattern);
        break;
    }
    case SearchKind::Value32BE: {
        searchFor = cutterValueAsHex(searchFor, true, 32);
        RzSearchBytesPattern *pattern =
                rz_search_parse_byte_pattern(searchFor.toUtf8().constData(), "value32.be");
        if (!pattern) {
            return searchRef;
        }
        hits = rz_core_search_bytes(core, userOpts.get(), pattern);
        break;
    }
    case SearchKind::Value32LE: {
        searchFor = cutterValueAsHex(searchFor, false, 32);
        RzSearchBytesPattern *pattern =
                rz_search_parse_byte_pattern(searchFor.toUtf8().constData(), "value32.le");
        if (!pattern) {
            return searchRef;
        }
        hits = rz_core_search_bytes(core, userOpts.get(), pattern);
        break;
    }
    case SearchKind::Value64BE: {
        searchFor = cutterValueAsHex(searchFor, true, 64);
        RzSearchBytesPattern *pattern =
                rz_search_parse_byte_pattern(searchFor.toUtf8().constData(), "value64.be");
        if (!pattern) {
            return searchRef;
        }
        hits = rz_core_search_bytes(core, userOpts.get(), pattern);
        break;
    }
    case SearchKind::Value64LE: {
        searchFor = cutterValueAsHex(searchFor, false, 64);
        RzSearchBytesPattern *pattern =
                rz_search_parse_byte_pattern(searchFor.toUtf8().constData(), "value64.le");
        if (!pattern) {
            return searchRef;
        }
        hits = rz_core_search_bytes(core, userOpts.get(), pattern);
        break;
    }
    case SearchKind::String:
    case SearchKind::StringCaseInsensitive: {
        const auto str = searchFor.toUtf8();
        hits = rz_core_search_string(core, userOpts.get(), str.constData(), str.size(),
                                     kind == SearchKind::StringCaseInsensitive
                                             ? RZ_REGEX_CASELESS | RZ_REGEX_LITERAL
                                             : RZ_REGEX_LITERAL,
                                     RZ_STRING_ENC_GUESS);
        break;
    }
    case SearchKind::StringRegexExtended:
        hits = rz_core_search_string(core, userOpts.get(), searchFor.toUtf8().constData(), 0,
                                     RZ_REGEX_EXTENDED, RZ_STRING_ENC_GUESS);
        break;
    case SearchKind::CryptographicMaterial:
        hits = rz_core_search_cryptographic_material(core, userOpts.get(),
                                                     RZ_SEARCH_COLLECTION_CRYPTOGRAPHIC_ALL);
        break;
    case SearchKind::MagicSignature:
        hits = rz_core_search_magic(core, userOpts.get(), nullptr);
        break;
    }

    RzListIter *it;
    RzSearchHit *hit;
    CutterRzListForeach (hits, it, RzSearchHit, hit) {
        SearchDescription exp;
        QString detail = fromOwnedCharPtr(rz_search_hit_detail_as_string(hit));

        exp.offset = hit->address;
        exp.size = hit->size;
        exp.data = cutterGetSearchHitData(core, kind, hit).trimmed();
        exp.detail = hit->hit_desc;
        if (!detail.isEmpty()) {
            exp.detail += " (" + detail + ")";
        }
        exp.detail += " ";
        exp.detail += rz_meta_get_string(core->analysis, RZ_META_TYPE_COMMENT, exp.offset);
        exp.detail = exp.detail.trimmed();

        searchRef << exp;
    }

    rz_list_free(hits);
    return searchRef;
}

QList<XrefDescription> CutterCore::getXRefsForVariable(QString variableName, bool findWrites,
                                                       RVA offset)
{
    CORE_LOCK();
    auto fcn = functionIn(offset);
    if (!fcn) {
        return {};
    }
    const auto typ =
            findWrites ? RZ_ANALYSIS_VAR_ACCESS_TYPE_WRITE : RZ_ANALYSIS_VAR_ACCESS_TYPE_READ;
    QList<XrefDescription> xrefList = QList<XrefDescription>();
    for (const auto &v : CutterPVector<RzAnalysisVar>(&fcn->vars)) {
        if (variableName != v->name) {
            continue;
        }
        RzAnalysisVarAccess *acc;
        CutterRzVectorForeach(&v->accesses, acc, RzAnalysisVarAccess)
        {
            if (!(acc->type & typ)) {
                continue;
            }
            XrefDescription xref;
            RVA addr = fcn->addr + acc->offset;
            xref.from = addr;
            xref.to = addr;
            if (findWrites) {
                xref.from_str = RzAddressString(addr);
            } else {
                xref.to_str = RzAddressString(addr);
            }
            xrefList << xref;
        }
    }
    return xrefList;
}

QList<XrefDescription> CutterCore::getXRefs(RVA addr, bool to, bool whole_function,
                                            const QString &filterType)
{
    QList<XrefDescription> xrefList = QList<XrefDescription>();

    RzList *xrefs = nullptr;
    {
        CORE_LOCK();
        if (to) {
            xrefs = rz_analysis_xrefs_get_to(core->analysis, addr);
        } else {
            xrefs = rz_analysis_xrefs_get_from(core->analysis, addr);
        }
    }

    RzListIter *it;
    RzAnalysisXRef *xref;
    CutterRzListForeach (xrefs, it, RzAnalysisXRef, xref) {
        XrefDescription xd;
        xd.from = xref->from;
        xd.to = xref->to;
        xd.type = rz_analysis_xrefs_type_tostring(xref->type);

        if (!filterType.isNull() && filterType != xd.type)
            continue;
        if (!whole_function && !to && xd.from != addr) {
            continue;
        }

        xd.from_str = RzAddressString(xd.from);
        xd.to_str = Core()->flagAt(xd.to);

        xrefList << xd;
    }
    rz_list_free(xrefs);
    return xrefList;
}

void CutterCore::addGlobalVariable(RVA offset, QString name, QString typ)
{
    name = sanitizeStringForCommand(name);
    CORE_LOCK();
    char *errmsg = NULL;
    RzType *globType = rz_type_parse_string_single(core->analysis->typedb->parser,
                                                   typ.toStdString().c_str(), &errmsg);
    if (errmsg) {
        qWarning() << tr("Error parsing type: \"%1\" message: ").arg(typ) << errmsg;
        free(errmsg);
        return;
    }
    if (!rz_analysis_var_global_create(core->analysis, name.toStdString().c_str(), globType,
                                       offset)) {
        qWarning() << tr("Error creating global variable: \"%1\"").arg(name);
        return;
    }

    emit globalVarsChanged();
}

void CutterCore::modifyGlobalVariable(RVA offset, QString name, QString typ)
{
    name = sanitizeStringForCommand(name);
    CORE_LOCK();
    RzAnalysisVarGlobal *glob = rz_analysis_var_global_get_byaddr_at(core->analysis, offset);
    if (!glob) {
        return;
    }
    // Compare if the name is not the same - also rename it
    if (name.compare(glob->name)) {
        rz_analysis_var_global_rename(core->analysis, glob->name, name.toStdString().c_str());
    }
    char *errmsg = NULL;
    RzType *globType = rz_type_parse_string_single(core->analysis->typedb->parser,
                                                   typ.toStdString().c_str(), &errmsg);
    if (errmsg) {
        qWarning() << tr("Error parsing type: \"%1\" message: ").arg(typ) << errmsg;
        free(errmsg);
        return;
    }
    rz_analysis_var_global_set_type(glob, globType);

    emit globalVarsChanged();
}

void CutterCore::delGlobalVariable(QString name)
{
    name = sanitizeStringForCommand(name);
    CORE_LOCK();
    rz_analysis_var_global_delete_byname(core->analysis, name.toStdString().c_str());

    emit globalVarsChanged();
}

void CutterCore::delGlobalVariable(RVA offset)
{
    CORE_LOCK();
    rz_analysis_var_global_delete_byaddr_at(core->analysis, offset);

    emit globalVarsChanged();
}

QString CutterCore::getGlobalVariableType(QString name)
{
    name = sanitizeStringForCommand(name);
    CORE_LOCK();
    RzAnalysisVarGlobal *glob =
            rz_analysis_var_global_get_byname(core->analysis, name.toStdString().c_str());
    if (!glob) {
        return QString("");
    }
    const char *gtype = rz_type_as_string(core->analysis->typedb, glob->type);
    if (!gtype) {
        return QString("");
    }
    return QString(gtype);
}

QString CutterCore::getGlobalVariableType(RVA offset)
{
    CORE_LOCK();
    RzAnalysisVarGlobal *glob = rz_analysis_var_global_get_byaddr_at(core->analysis, offset);
    if (!glob) {
        return QString("");
    }
    const char *gtype = rz_type_as_string(core->analysis->typedb, glob->type);
    if (!gtype) {
        return QString("");
    }
    return QString(gtype);
}

void CutterCore::addFlag(RVA offset, QString name, RVA size)
{
    name = sanitizeStringForCommand(name);
    CORE_LOCK();
    rz_flag_set(core->flags, name.toStdString().c_str(), offset, size);
    emit flagsChanged();
}

/**
 * @brief Gets all the flags present at a specific address
 * @param addr The address to be checked
 * @return String containing all the flags which are comma-separated
 */
QString CutterCore::listFlagsAsStringAt(RVA addr)
{
    CORE_LOCK();
    char *flagList = rz_flag_get_liststr(core->flags, addr);
    QString result = fromOwnedCharPtr(flagList);
    return result;
}

QString CutterCore::nearestFlag(RVA offset, RVA *flagOffsetOut)
{
    CORE_LOCK();
    auto r = rz_flag_get_at(core->flags, offset, true);
    if (!r) {
        return {};
    }
    if (flagOffsetOut) {
        *flagOffsetOut = r->offset;
    }
    return r->name;
}

void CutterCore::addMark(RVA from, RVA to, QString name, QString comment, QColor color)
{
    CORE_LOCK();
    auto m = rz_mark_set(core->marks, name.toStdString().c_str(), from, to);
    if (m) {
        rz_mark_item_set_comment(m, comment.toStdString().c_str());
        rz_mark_item_set_color(m, color.name().toStdString().c_str());
    }
    emit marksChanged();
}

void CutterCore::delMark(const QString &name)
{
    CORE_LOCK();
    auto m = rz_mark_get(core->marks, name.toStdString().c_str());
    if (m) {
        rz_mark_unset(core->marks, m);
    }
    emit marksChanged();
}

QList<MarkDescription> CutterCore::convertMarks(RzList *marks)
{
    QList<MarkDescription> markList;

    RzListIter *it;
    RzMarkItem *mark;
    CutterRzListForeach (marks, it, RzMarkItem, mark) {
        MarkDescription desc;
        desc.from = mark->from;
        desc.to = mark->to;
        desc.name = mark->name;
        desc.realname = mark->realname;
        desc.comment = mark->comment;
        desc.color = mark->color ? QColor(mark->color) : QColor(Qt::black);

        markList.append(desc);
    }
    rz_list_free(marks);
    return markList;
}

QList<MarkDescription> CutterCore::getMarks()
{
    CORE_LOCK();
    return convertMarks(rz_mark_all_list(core->marks));
}

QList<MarkDescription> CutterCore::getMarksAt(RVA addr)
{
    CORE_LOCK();
    return convertMarks(rz_mark_get_all_off(core->marks, addr));
}

QColor CutterCore::getBlendedMarksColorAt(RVA addr)
{
    const auto &marks = getMarksAt(addr);
    double r = 0, g = 0, b = 0, a = 0;
    bool first = true;

    // Iterate in reverse because the oldest/first mark is at the end
    for (auto it = marks.crbegin(); it != marks.crend(); ++it) {
        QColor c = it->color;
        if (!c.isValid()) {
            continue;
        }

        double cr = c.redF(), cg = c.greenF(), cb = c.blueF();
        if (first) {
            r = cr, g = cg, b = cb, a = MARK_ALPHA_F;
            first = false;
        } else {
            double a_out = MARK_ALPHA_F + a * (1.0 - MARK_ALPHA_F);
            r = (cr * MARK_ALPHA_F + r * a * (1.0 - MARK_ALPHA_F)) / a_out;
            g = (cg * MARK_ALPHA_F + g * a * (1.0 - MARK_ALPHA_F)) / a_out;
            b = (cb * MARK_ALPHA_F + b * a * (1.0 - MARK_ALPHA_F)) / a_out;
            a = a_out;
        }
    }
    return first ? QColor() : QColor::fromRgbF(r, g, b, a);
}

void CutterCore::handleREvent(int type, void *data)
{
    switch (type) {
    case RZ_EVENT_CLASS_NEW: {
        auto ev = reinterpret_cast<RzEventClass *>(data);
        emit classNew(QString::fromUtf8(ev->name));
        break;
    }
    case RZ_EVENT_CLASS_DEL: {
        auto ev = reinterpret_cast<RzEventClass *>(data);
        emit classDeleted(QString::fromUtf8(ev->name));
        break;
    }
    case RZ_EVENT_CLASS_RENAME: {
        auto ev = reinterpret_cast<RzEventClassRename *>(data);
        emit classRenamed(QString::fromUtf8(ev->name_old), QString::fromUtf8(ev->name_new));
        break;
    }
    case RZ_EVENT_CLASS_ATTR_SET: {
        auto ev = reinterpret_cast<RzEventClassAttrSet *>(data);
        emit classAttrsChanged(QString::fromUtf8(ev->attr.class_name));
        break;
    }
    case RZ_EVENT_CLASS_ATTR_DEL: {
        auto ev = reinterpret_cast<RzEventClassAttr *>(data);
        emit classAttrsChanged(QString::fromUtf8(ev->class_name));
        break;
    }
    case RZ_EVENT_CLASS_ATTR_RENAME: {
        auto ev = reinterpret_cast<RzEventClassAttrRename *>(data);
        emit classAttrsChanged(QString::fromUtf8(ev->attr.class_name));
        break;
    }
    case RZ_EVENT_DEBUG_PROCESS_FINISHED: {
        auto ev = reinterpret_cast<RzEventDebugProcessFinished *>(data);
        emit debugProcessFinished(ev->pid);
        break;
    }
    default:
        break;
    }
}

void CutterCore::triggerFlagsChanged()
{
    emit flagsChanged();
}

void CutterCore::triggerVarsChanged()
{
    emit varsChanged();
}

void CutterCore::triggerFunctionRenamed(const RVA offset, const QString &newName)
{
    emit functionRenamed(offset, newName);
}

void CutterCore::loadPDB(const QString &file)
{
    CORE_LOCK();
    rz_core_bin_pdb_load(core, file.toUtf8().constData());
}

QList<DisassemblyLine> CutterCore::disassembleLines(RVA offset, int lines)
{
    CORE_LOCK();
    auto vec = fromOwned(
            rz_pvector_new(reinterpret_cast<RzPVectorFree>(rz_analysis_disasm_text_free)));
    if (!vec) {
        return {};
    }

    RzCoreDisasmOptions options = {};
    options.cbytes = 1;
    options.vec = vec.get();
    {
        auto restoreSeek = seekTemp(offset);
        if (rz_cons_singleton()->is_html) {
            rz_cons_singleton()->is_html = false;
            rz_cons_singleton()->was_html = true;
        }
        rz_core_print_disasm(core, offset, core->block, core->blocksize, lines, NULL, &options);
    }

    QList<DisassemblyLine> r;
    for (const auto &t : CutterPVector<RzAnalysisDisasmText>(vec.get())) {
        QString text = t->text;
        QStringList tokens = text.split('\n');
        // text might contain multiple lines
        // so we split them and keep only one
        // arrow/jump to addr.
        for (const auto &tok : tokens) {
            DisassemblyLine line;
            line.offset = t->offset;
            line.text = ansiEscapeToHtml(tok);
            line.arrow = t->arrow;
            r << line;
            // only the first one.
            t->arrow = RVA_INVALID;
        }
    }
    return r;
}

/**
 * @brief return hexdump of <size> from an <offset> by a given formats
 * @param address - the address from which to print the hexdump
 * @param size - number of bytes to print
 * @param format - the type of hexdump (qwords, words. decimal, etc)
 */
QString CutterCore::hexdump(RVA address, int size, HexdumpFormats format)
{
    CORE_LOCK();
    char *res = nullptr;
    switch (format) {
    case HexdumpFormats::Normal:
        res = rz_core_print_hexdump_or_hexdiff_str(core, RZ_OUTPUT_MODE_STANDARD, address, size,
                                                   false);
        break;
    case HexdumpFormats::Half:
        res = rz_core_print_dump_str(core, RZ_OUTPUT_MODE_STANDARD, address, 2, size,
                                     RZ_CORE_PRINT_FORMAT_TYPE_HEXADECIMAL);
        break;
    case HexdumpFormats::Word:
        res = rz_core_print_dump_str(core, RZ_OUTPUT_MODE_STANDARD, address, 4, size,
                                     RZ_CORE_PRINT_FORMAT_TYPE_HEXADECIMAL);
        break;
    case HexdumpFormats::Quad:
        res = rz_core_print_dump_str(core, RZ_OUTPUT_MODE_STANDARD, address, 8, size,
                                     RZ_CORE_PRINT_FORMAT_TYPE_HEXADECIMAL);
        break;
    case HexdumpFormats::Signed:
        res = rz_core_print_dump_str(core, RZ_OUTPUT_MODE_STANDARD, address, 1, size,
                                     RZ_CORE_PRINT_FORMAT_TYPE_INTEGER);
        break;
    case HexdumpFormats::Octal:
        res = rz_core_print_dump_str(core, RZ_OUTPUT_MODE_STANDARD, address, 1, size,
                                     RZ_CORE_PRINT_FORMAT_TYPE_OCTAL);
        break;
    }

    return fromOwnedCharPtr(res);
}

QByteArray CutterCore::hexStringToBytes(const QString &hex)
{
    QByteArray hexChars = hex.toUtf8();
    QByteArray bytes;
    bytes.reserve(hexChars.length() / 2);
    int size = rz_hex_str2bin(hexChars.constData(), reinterpret_cast<ut8 *>(bytes.data()));
    bytes.resize(size);
    return bytes;
}

QString CutterCore::bytesToHexString(const QByteArray &bytes)
{
    return QString::fromUtf8(bytes.toHex());
}

void CutterCore::loadScript(const QString &scriptname)
{
    {
        CORE_LOCK();
        rz_core_cmd_file(core, scriptname.toUtf8().constData());
        rz_cons_flush();
    }
    triggerRefreshAll();
}

QString CutterCore::getRizinVersionReadable(const char *program)
{
    return fromOwnedCharPtr(rz_version_str(core_->sys_path, program));
}

QString CutterCore::getVersionInformation()
{
    int i;
    QString versionInfo;
    struct vcs_t
    {
        const char *name;
        const char *(*callback)();
    } vcs[] = {
        { "rz_arch", &rz_arch_version },
        { "rz_lib", &rz_lib_version },
        { "rz_egg", &rz_egg_version },
        { "rz_bin", &rz_bin_version },
        { "rz_cons", &rz_cons_version },
        { "rz_flag", &rz_flag_version },
        { "rz_core", &rz_core_version },
        { "rz_crypto", &rz_crypto_version },
        { "rz_debug", &rz_debug_version },
        { "rz_hash", &rz_hash_version },
        { "rz_io", &rz_io_version },
#if !USE_LIB_MAGIC
        { "rz_magic", &rz_magic_version },
#endif
        { "rz_reg", &rz_reg_version },
        { "rz_sign", &rz_sign_version },
        { "rz_search", &rz_search_version },
        { "rz_syscall", &rz_syscall_version },
        { "rz_util", &rz_util_version },
        /* ... */
        { NULL, NULL }
    };
    versionInfo.append(getRizinVersionReadable());
    versionInfo.append("\n");
    for (i = 0; vcs[i].name; i++) {
        struct vcs_t *v = &vcs[i];
        const char *name = v->callback();
        versionInfo.append(QString("%1 %2\n").arg(name, v->name));
    }
    return versionInfo;
}

QStringList CutterCore::getColorThemes()
{
    QStringList r;
    CORE_LOCK();
    RzPVector *themes_list = rz_core_get_themes(core);
    if (!themes_list) {
        return r;
    }
    for (const auto &th : CutterPVector<char>(themes_list)) {
        r << fromOwnedCharPtr(rz_str_trim_dup(th));
    }
    rz_pvector_free(themes_list);
    return r;
}

QHash<QString, QColor> CutterCore::getTheme()
{
    QHash<QString, QColor> theme;
    for (int i = 0;; ++i) {
        const char *k = rz_cons_pal_get_name(i);
        if (!k) {
            break;
        }
        RzColor color = rz_cons_pal_get_i(i);
        theme.insert(k, QColor(color.r, color.g, color.b));
    }
    return theme;
}

QStringList CutterCore::getThemeKeys()
{
    QStringList stringList;
    for (int i = 0;; ++i) {
        const char *k = rz_cons_pal_get_name(i);
        if (!k) {
            break;
        }
        stringList << k;
    }
    return stringList;
}

bool CutterCore::setColor(const QString &key, const QString &color)
{
    if (!rz_cons_pal_set(key.toUtf8().constData(), color.toUtf8().constData())) {
        return false;
    }
    rz_cons_pal_update_event();
    return true;
}

QString CutterCore::ansiEscapeToHtml(const QString &text)
{
    int len;
    QString r = text;
    r.replace("\t", "        ");
    char *html = rz_cons_html_filter(r.toUtf8().constData(), &len);
    if (!html) {
        return {};
    }
    r = QString::fromUtf8(html, len);
    rz_mem_free(html);
    return r;
}

BasicBlockHighlighter *CutterCore::getBBHighlighter()
{
    return bbHighlighter;
}

BasicInstructionHighlighter *CutterCore::getBIHighlighter()
{
    return &biHighlighter;
}

void CutterCore::setIOCache(bool enabled)
{
    if (enabled) {
        // disable write mode when cache is enabled
        setWriteMode(false);
    }
    setConfig("io.cache", enabled);
    this->iocache = enabled;

    emit ioCacheChanged(enabled);
    emit ioModeChanged();
}

bool CutterCore::isIOCacheEnabled() const
{
    return iocache;
}

void CutterCore::commitWriteCache()
{
    CORE_LOCK();
    // Temporarily disable cache mode
    TempConfig tempConfig;
    tempConfig.set("io.cache", false);
    auto desc = core->io->desc;
    bool reopen = !isWriteModeEnabled() && desc;
    if (reopen) {
        rz_core_io_file_reopen(core, desc->fd, RZ_PERM_RW);
    }
    rz_io_cache_commit(core->io, 0, UT64_MAX);
    rz_core_block_read(core);
    if (reopen) {
        rz_core_io_file_open(core, desc->fd);
    }
}

void CutterCore::resetWriteCache()
{
    CORE_LOCK();
    rz_io_cache_reset(core->io, core->io->cached);
}

// Enable or disable write-mode. Avoid unecessary changes if not need.
void CutterCore::setWriteMode(bool enabled)
{
    bool writeModeState = isWriteModeEnabled();

    if (writeModeState == enabled && !this->iocache) {
        // New mode is the same as current and IO Cache is disabled. Do nothing.
        return;
    }

    CORE_LOCK();
    // Change from read-only to write-mode
    RzIODesc *desc = core->io->desc;
    if (desc) {
        if (enabled) {
            if (!writeModeState) {
                rz_core_io_file_reopen(core, desc->fd, RZ_PERM_RW);
            }
        } else {
            // Change from write-mode to read-only
            rz_core_io_file_open(core, desc->fd);
        }
    }
    // Disable cache mode because we specifically set write or
    // read-only modes.
    if (this->iocache) {
        setIOCache(false);
    }
    emit writeModeChanged(enabled);
    emit ioModeChanged();
}

bool CutterCore::isWriteModeEnabled()
{
    CORE_LOCK();
    RzListIter *it;
    RzCoreFile *cf;
    CutterRzListForeach (core->files, it, RzCoreFile, cf) {
        RzIODesc *desc = rz_io_desc_get(core->io, cf->fd);
        if (!desc) {
            continue;
        }
        if (desc->perm & RZ_PERM_W) {
            return true;
        }
    }
    return false;
}

/**
 * @brief get a compact disassembly preview for tooltips
 * @param address - the address from which to print the disassembly
 * @param num_of_lines - number of instructions to print
 */
QStringList CutterCore::getDisassemblyPreview(RVA address, int num_of_lines)
{
    QList<DisassemblyLine> disassemblyLines;
    {
        // temporarily simplify the disasm output to get it colorful and simple to read
        TempConfig tempConfig;
        tempConfig.set("scr.color", COLOR_MODE_16M)
                .set("asm.lines", false)
                .set("asm.var", false)
                .set("asm.comments", false)
                .set("asm.bytes", false)
                .set("asm.lines.fcn", false)
                .set("asm.lines.out", false)
                .set("asm.lines.bb", false)
                .set("asm.bb.line", false);

        disassemblyLines = disassembleLines(address, num_of_lines + 1);
    }
    QStringList disasmPreview;
    for (const DisassemblyLine &line : disassemblyLines) {
        disasmPreview << line.text;
        if (disasmPreview.length() >= num_of_lines) {
            disasmPreview << "...";
            break;
        }
    }
    if (!disasmPreview.isEmpty()) {
        return disasmPreview;
    } else {
        return QStringList();
    }
}

/**
 * @brief get a compact hexdump preview for tooltips
 * @param address - the address from which to print the hexdump
 * @param size - number of bytes to print
 */
QString CutterCore::getHexdumpPreview(RVA address, int size)
{
    // temporarily simplify the disasm output to get it colorful and simple to read
    TempConfig tempConfig;
    tempConfig.set("scr.color", COLOR_MODE_16M)
            .set("asm.offset", true)
            .set("hex.header", false)
            .set("hex.cols", 16);
    return ansiEscapeToHtml(hexdump(address, size, HexdumpFormats::Normal))
            .replace(QLatin1Char('\n'), "<br>");
}

QByteArray CutterCore::ioRead(RVA addr, int len)
{
    CORE_LOCK();

    QByteArray array;

    if (len <= 0)
        return array;

    /* Zero-copy */
    array.resize(len);
    if (!rz_io_read_at(core->io, addr, (uint8_t *)array.data(), len)) {
        array.fill(0xff);
    }

    return array;
}

QStringList CutterCore::getConfigVariableSpaces(const QString &key)
{
    CORE_LOCK();
    RzList *list = rz_core_config_in_space(core, key.toUtf8().constData());
    if (!list) {
        return {};
    }

    QStringList stringList;
    for (const auto &x : CutterRzList<char>(list)) {
        stringList << x;
    }
    rz_list_free(list);
    return stringList;
}

char *CutterCore::getTextualGraphAt(RzCoreGraphType type, RzCoreGraphFormat format, RVA address)
{
    CORE_LOCK();
    char *string = nullptr;
    RzGraph *graph = rz_core_graph(core, type, address);
    if (!graph) {
        if (address == RVA_INVALID) {
            qWarning() << tr("Cannot get global graph");
        } else {
            qWarning() << tr("Cannot get graph at ") << RzAddressString(address);
        }
        return nullptr;
    }
    core->graph->is_callgraph = type == RZ_CORE_GRAPH_TYPE_FUNCALL;

    switch (format) {
    case RZ_CORE_GRAPH_FORMAT_CMD: {
        string = rz_graph_drawable_to_cmd(graph);
        break;
    }
    case RZ_CORE_GRAPH_FORMAT_DOT: {
        string = rz_core_graph_to_dot_str(core, graph);
        break;
    }
    case RZ_CORE_GRAPH_FORMAT_JSON:
        /* fall-thru */
    case RZ_CORE_GRAPH_FORMAT_JSON_DISASM: {
        string = rz_graph_drawable_to_json_str(graph, true);
        break;
    }
    case RZ_CORE_GRAPH_FORMAT_GML: {
        string = rz_graph_drawable_to_gml(graph);
        break;
    }
    default:
        break;
    }
    rz_graph_free(graph);

    if (!string) {
        qWarning() << tr("Failed to generate graph");
    }

    return string;
}

void CutterCore::writeGraphvizGraphToFile(QString path, QString format, RzCoreGraphType type,
                                          RVA address)
{
    TempConfig tempConfig;
    tempConfig.set("scr.color", false);
    tempConfig.set("graph.gv.format", format);

    CORE_LOCK();
    auto filepath = path.toUtf8();

    if (!rz_core_graph_write(core, address, type, filepath)) {
        if (address == RVA_INVALID) {
            qWarning() << tr("Cannot get global graph");
        } else {
            qWarning() << tr("Cannot get graph at ") << RzAddressString(address);
        }
    }
}
