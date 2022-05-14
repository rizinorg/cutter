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

#include "common/TempConfig.h"
#include "common/BasicInstructionHighlighter.h"
#include "common/Configuration.h"
#include "common/AsyncTask.h"
#include "common/RizinTask.h"
#include "dialogs/RizinTaskDialog.h"
#include "common/Json.h"
#include "core/Cutter.h"
#include "Decompiler.h"

#include <rz_asm.h>
#include <rz_cmd.h>
#include <sdb.h>

Q_GLOBAL_STATIC(CutterCore, uniqueInstance)

#define RZ_JSON_KEY(name) static const QString name = QStringLiteral(#name)

namespace RJsonKey {
RZ_JSON_KEY(addr);
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

static QString fromOwnedCharPtr(char *str)
{
    QString result(str ? str : "");
    rz_mem_free(str);
    return result;
}

static bool reg_sync(RzCore *core, RzRegisterType type, bool write)
{
    return rz_debug_reg_sync(core->dbg, type, write);
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

RzCoreLocked::operator RzCore *() const
{
    return core->core_;
}

RzCore *RzCoreLocked::operator->() const
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
}

CutterCore *CutterCore::instance()
{
    return uniqueInstance;
}

void CutterCore::initialize(bool loadPlugins)
{
    auto prefix = QDir(QCoreApplication::applicationDirPath());

#if defined(CUTTER_ENABLE_PACKAGING) && defined(Q_OS_WIN)
    auto prefixBytes = prefix.absolutePath().toUtf8();
    rz_path_prefix(prefixBytes.constData());
#endif

    rz_cons_new(); // initialize console
    core_ = rz_core_new();
    rz_core_task_sync_begin(&core_->tasks);
    coreBed = rz_cons_sleep_begin();
    CORE_LOCK();

    rz_event_hook(core_->analysis->ev, RZ_EVENT_ALL, cutterREventCallback, this);

#if defined(APPIMAGE) || defined(MACOS_RZ_BUNDLED)
#    ifdef APPIMAGE
    // Executable is in appdir/bin
    prefix.cdUp();
    qInfo() << "Setting Rizin prefix =" << prefix.absolutePath() << " for AppImage.";
#    else // MACOS_RZ_BUNDLED
    // Executable is in Contents/MacOS, prefix is Contents/Resources/rz
    prefix.cdUp();
    prefix.cd("Resources");
    qInfo() << "Setting Rizin prefix =" << prefix.absolutePath()
            << " for macOS Application Bundle.";
    setConfig("dir.prefix", prefix.absolutePath());
#    endif

    auto pluginsDir = prefix;
    if (pluginsDir.cd("share/rizin/plugins")) {
        qInfo() << "Setting Rizin plugins dir =" << pluginsDir.absolutePath();
        setConfig("dir.plugins", pluginsDir.absolutePath());
    } else {
        qInfo() << "Rizin plugins dir under" << pluginsDir.absolutePath() << "does not exist!";
    }
#endif

    if (!loadPlugins) {
        setConfig("cfg.plugins", 0);
    }
    if (getConfigi("cfg.plugins")) {
        rz_core_loadlibs(this->core_, RZ_CORE_LOADLIBS_ALL);
    }
    // IMPLICIT rz_bin_iobind (core_->bin, core_->io);

    // Otherwise Rizin may ask the user for input and Cutter would freeze
    setConfig("scr.interactive", false);

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
}

RzCoreLocked CutterCore::core()
{
    return RzCoreLocked(this);
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
        qInfo() << "Loading initialization file from " << cutterRCFilePath;
        rz_core_cmd_file(core, cutterRCFilePath.toUtf8().constData());
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
    qInfo() << "Loading initialization file from " << cutterRCFilePath;
    rz_core_cmd_file(core, cutterRCFilePath.toUtf8().constData());
}

QList<QString> CutterCore::sdbList(QString path)
{
    CORE_LOCK();
    QList<QString> list = QList<QString>();
    Sdb *root = sdb_ns_path(core->sdb, path.toUtf8().constData(), 0);
    if (root) {
        void *vsi;
        ls_iter_t *iter;
        ls_foreach(root->ns, iter, vsi)
        {
            SdbNs *nsi = (SdbNs *)vsi;
            list << nsi->name;
        }
    }
    return list;
}

using SdbListPtr = std::unique_ptr<SdbList, decltype(&ls_free)>;
static SdbListPtr makeSdbListPtr(SdbList *list)
{
    return { list, ls_free };
}

QList<QString> CutterCore::sdbListKeys(QString path)
{
    CORE_LOCK();
    QList<QString> list = QList<QString>();
    Sdb *root = sdb_ns_path(core->sdb, path.toUtf8().constData(), 0);
    if (root) {
        void *vsi;
        ls_iter_t *iter;
        SdbListPtr l = makeSdbListPtr(sdb_foreach_list(root, false));
        ls_foreach(l, iter, vsi)
        {
            SdbKv *nsi = (SdbKv *)vsi;
            list << reinterpret_cast<char *>(nsi->base.key);
        }
    }
    return list;
}

QString CutterCore::sdbGet(QString path, QString key)
{
    CORE_LOCK();
    Sdb *db = sdb_ns_path(core->sdb, path.toUtf8().constData(), 0);
    if (db) {
        const char *val = sdb_const_get(db, key.toUtf8().constData(), 0);
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
    return sdb_set(db, key.toUtf8().constData(), val.toUtf8().constData(), 0);
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

bool CutterCore::asyncCmdEsil(const char *command, QSharedPointer<RizinTask> &task)
{
    asyncCmd(command, task);

    if (task.isNull()) {
        return false;
    }

    connect(task.data(), &RizinCmdTask::finished, task.data(), [this, task]() {
        QString res = qobject_cast<RizinCmdTask *>(task.data())->getResult();

        if (res.contains(QStringLiteral("[ESIL] Stopped execution in an invalid instruction"))) {
            msgBox.showMessage("Stopped when attempted to run an invalid instruction. You can "
                               "disable this in Preferences");
        }
    });

    return true;
}

bool CutterCore::asyncCmd(const char *str, QSharedPointer<RizinTask> &task)
{
    if (!task.isNull()) {
        return false;
    }

    CORE_LOCK();

    RVA offset = core->offset;

    task = QSharedPointer<RizinTask>(new RizinCmdTask(str, true));
    connect(task.data(), &RizinTask::finished, task.data(), [this, offset, task]() {
        CORE_LOCK();

        if (offset != core->offset) {
            updateSeek();
        }
    });

    return true;
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
    rz_cons_push();

    // rz_cmd_call does not return the output of the command
    rz_cmd_call(core->rcmd, cmd);

    // we grab the output straight from rz_cons
    res = rz_cons_get_buffer();

    // cleaning up
    rz_cons_pop();
    rz_cons_echo(NULL);

    return res;
}

CutterJson CutterCore::cmdj(const char *str)
{
    char *res;
    {
        CORE_LOCK();
        res = rz_core_cmd_str(core, str);
    }

    return parseJson(res, str);
}

CutterJson CutterCore::cmdjAt(const char *str, RVA address)
{
    CutterJson res;
    RVA oldOffset = getOffset();
    seekSilent(address);

    res = cmdj(str);

    seekSilent(oldOffset);
    return res;
}

QString CutterCore::cmdTask(const QString &str)
{
    RizinCmdTask task(str);
    task.startTask();
    task.joinTask();
    return task.getResult();
}

CutterJson CutterCore::cmdjTask(const QString &str)
{
    RizinCmdTask task(str);
    task.startTask();
    task.joinTask();
    const char *res = task.getResultRaw();
    char *copy = static_cast<char *>(rz_mem_alloc(strlen(res) + 1));
    strcpy(copy, res);
    return parseJson(copy, str);
}

CutterJson CutterCore::parseJson(char *res, const char *cmd)
{
    if (!res) {
        return CutterJson();
    }

    RzJson *doc = rz_json_parse(res);

    if (!doc) {
        if (cmd) {
            eprintf("Failed to parse JSON for command \"%s\"\n", cmd);
        } else {
            eprintf("Failed to parse JSON\n");
        }
        const int MAX_JSON_DUMP_SIZE = 8 * 1024;
        size_t originalSize = strlen(res);
        if (originalSize > MAX_JSON_DUMP_SIZE) {
            res[MAX_JSON_DUMP_SIZE] = 0;
            eprintf("%zu bytes total: %s ...\n", originalSize, res);
        } else {
            eprintf("%s\n", res);
        }
    }

    return CutterJson(doc, QSharedPointer<CutterJsonOwner>::create(doc, res));
}

QStringList CutterCore::autocomplete(const QString &cmd, RzLinePromptType promptType, size_t limit)
{
    RzLineBuffer buf;
    int c = snprintf(buf.data, sizeof(buf.data), "%s", cmd.toUtf8().constData());
    if (c < 0) {
        return {};
    }
    buf.index = buf.length = std::min((int)(sizeof(buf.data) - 1), c);

    RzLineCompletion completion;
    rz_line_completion_init(&completion, limit);
    rz_core_autocomplete(core(), &completion, &buf, promptType);

    QStringList r;
    r.reserve(rz_pvector_len(&completion.args));
    for (size_t i = 0; i < rz_pvector_len(&completion.args); i++) {
        r.push_back(QString::fromUtf8(
                reinterpret_cast<const char *>(rz_pvector_at(&completion.args, i))));
    }

    rz_line_completion_fini(&completion);
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
    } else {
        // Not loading RzBin info coz va = false
    }

    auto iod = core->io ? core->io->desc : NULL;
    auto debug =
            core->file && iod && (core->file->fd == iod->fd) && iod->plugin && iod->plugin->isdbg;

    if (!debug && rz_flag_get(core->flags, "entry0")) {
        rz_core_cmd0(core, "s entry0");
    }

    if (perms & RZ_PERM_W) {
        rz_core_cmd0(core, "omfg+w");
    }

    fflush(stdout);
    return true;
}

bool CutterCore::tryFile(QString path, bool rw)
{
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
    ut64 baddr = Core()->getFileInfo()["bin"]["baddr"].toUt64();
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

QString CutterCore::getInstructionBytes(RVA addr)
{
    auto ret = (char *)Core()->returnAtSeek(
            [&]() {
                CORE_LOCK();
                RzPVector *vec = rz_core_analysis_bytes(core, core->block, (int)core->blocksize, 1);
                auto *ab = static_cast<RzAnalysisBytes *>(rz_pvector_head(vec));
                char *str = strdup(ab->bytes);
                rz_pvector_free(vec);
                return str;
            },
            addr);
    return fromOwnedCharPtr(ret);
}

QString CutterCore::getInstructionOpcode(RVA addr)
{
    auto ret = (char *)Core()->returnAtSeek(
            [&]() {
                CORE_LOCK();
                RzPVector *vec = rz_core_analysis_bytes(core, core->block, (int)core->blocksize, 1);
                auto *ab = static_cast<RzAnalysisBytes *>(rz_pvector_head(vec));
                char *str = strdup(ab->opcode);
                rz_pvector_free(vec);
                return str;
            },
            addr);
    return fromOwnedCharPtr(ret);
}

void CutterCore::editInstruction(RVA addr, const QString &inst)
{
    CORE_LOCK();
    rz_core_write_assembly(core, addr, inst.trimmed().toStdString().c_str());
    emit instructionChanged(addr);
}

void CutterCore::nopInstruction(RVA addr)
{
    CORE_LOCK();
    applyAtSeek([&]() { rz_core_hack(core, "nop"); }, addr);
    emit instructionChanged(addr);
}

void CutterCore::jmpReverse(RVA addr)
{
    CORE_LOCK();
    applyAtSeek([&]() { rz_core_hack(core, "recj"); }, addr);
    emit instructionChanged(addr);
}

void CutterCore::editBytes(RVA addr, const QString &bytes)
{
    cmdRawAt(QString("wx %1").arg(bytes), addr);
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
    char *s = (char *)returnAtSeek(
            [&]() {
                RzStrStringifyOpt opt = { 0 };
                opt.buffer = core->block;
                opt.length = core->blocksize;
                opt.encoding = rz_str_guess_encoding_from_buffer(core->block, core->blocksize);
                return rz_str_stringify_raw_buffer(&opt, NULL);
            },
            addr);
    return fromOwnedCharPtr(s);
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

    this->cmdRawAt(QString("ahi %1").arg(rzBaseName), offset);
    emit instructionChanged(offset);
}

void CutterCore::setCurrentBits(int bits, RVA offset)
{
    if (offset == RVA_INVALID) {
        offset = getOffset();
    }

    this->cmdRawAt(QString("ahb %1").arg(bits), offset);
    emit instructionChanged(offset);
}

void CutterCore::applyStructureOffset(const QString &structureOffset, RVA offset)
{
    if (offset == RVA_INVALID) {
        offset = getOffset();
    }

    applyAtSeek(
            [&]() {
                CORE_LOCK();
                rz_core_analysis_hint_set_offset(core, structureOffset.toUtf8().constData());
            },
            offset);
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
    updateSeek();
}

void CutterCore::seekNext()
{
    CORE_LOCK();
    rz_core_seek_redo(core);
    updateSeek();
}

void CutterCore::updateSeek()
{
    emit seekChanged(getOffset());
}

RVA CutterCore::prevOpAddr(RVA startAddr, int count)
{
    CORE_LOCK();
    bool ok;
    RVA offset = cmdRawAt(QString("/O %1").arg(count), startAddr).toULongLong(&ok, 16);
    return ok ? offset : startAddr - count;
}

RVA CutterCore::nextOpAddr(RVA startAddr, int count)
{
    CORE_LOCK();

    CutterJson array =
            Core()->cmdj("pdj " + QString::number(count + 1) + " @ " + QString::number(startAddr));
    if (!array.size()) {
        return startAddr + 1;
    }

    CutterJson instValue = array.last();
    if (instValue.type() != RZ_JSON_OBJECT) {
        return startAddr + 1;
    }

    RVA offset = instValue[RJsonKey::offset].toRVA();
    if (offset == RVA_INVALID) {
        return startAddr + 1;
    }

    return offset;
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
    return QString(rz_config_get(core->config, k));
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
    return cmdRawAt("pi 1", addr).simplified();
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
    RzAnalysisBlock *lastBB = (RzAnalysisBlock *)rz_list_last(fcn->bbs);
    return lastBB ? rz_analysis_block_get_op_addr(lastBB, lastBB->ninstr - 1) : RVA_INVALID;
}

QString CutterCore::cmdFunctionAt(QString addr)
{
    QString ret;
    // Use cmd because cmdRaw would not work with grep
    ret = cmd(QString("fd @ %1~[0]").arg(addr));
    return ret.trimmed();
}

QString CutterCore::cmdFunctionAt(RVA addr)
{
    return cmdFunctionAt(QString::number(addr));
}

void CutterCore::cmdEsil(const char *command)
{
    // use cmd and not cmdRaw because of unexpected commands
    QString res = cmd(command);
    if (res.contains(QStringLiteral("[ESIL] Stopped execution in an invalid instruction"))) {
        msgBox.showMessage("Stopped when attempted to run an invalid instruction. You can disable "
                           "this in Preferences");
    }
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
    auto rva = (RVA *)Core()->returnAtSeek(
            [&]() {
                CORE_LOCK();
                RzPVector *vec = rz_core_analysis_bytes(core, core->block, (int)core->blocksize, 1);
                auto *ab = static_cast<RzAnalysisBytes *>(rz_pvector_head(vec));
                RVA *rva = new RVA(ab->op->jump);
                rz_pvector_free(vec);
                return rva;
            },
            addr);
    RVA ret = *rva;
    delete rva;
    return ret;
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

CutterJson CutterCore::getFileInfo()
{
    return cmdj("ij");
}

CutterJson CutterCore::getFileVersionInfo()
{
    return cmdj("iVj");
}

CutterJson CutterCore::getSignatureInfo()
{
    return cmdj("iCj");
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

QList<RegisterRef> CutterCore::getRegisterRefs(int depth)
{
    QList<RegisterRef> ret;
    if (!currentlyDebugging) {
        return ret;
    }

    CORE_LOCK();
    RzList *ritems = rz_core_reg_filter_items_sync(core, core->dbg->reg, reg_sync, nullptr);
    if (!ritems) {
        return ret;
    }
    RzListIter *it;
    RzRegItem *ri;
    CutterRzListForeach (ritems, it, RzRegItem, ri) {
        RegisterRef reg;
        reg.value = rz_reg_get_value(core->dbg->reg, ri);
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
    RVA addr = rz_debug_reg_get(core->dbg, "SP");
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
        RzRegItem *r = rz_reg_get(core->dbg->reg, fi->name, -1);
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
        m_arena = ((RzArenaListItem *)arenas->head->data)->addr;
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
        switch (var->kind) {
        case RZ_ANALYSIS_VAR_KIND_BPV:
            desc.refType = VariableDescription::RefType::BP;
            break;
        case RZ_ANALYSIS_VAR_KIND_SPV:
            desc.refType = VariableDescription::RefType::SP;
            break;
        case RZ_ANALYSIS_VAR_KIND_REG:
        default:
            desc.refType = VariableDescription::RefType::Reg;
            break;
        }
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
        ret.push_back(desc);
    }
    return ret;
}

QVector<RegisterRefValueDescription> CutterCore::getRegisterRefValues()
{
    QVector<RegisterRefValueDescription> result;
    CORE_LOCK();
    RzList *ritems = rz_core_reg_filter_items_sync(core, core->dbg->reg, reg_sync, nullptr);
    if (!ritems) {
        return result;
    }
    RzListIter *it;
    RzRegItem *ri;
    CutterRzListForeach (ritems, it, RzRegItem, ri) {
        RegisterRefValueDescription desc;
        desc.name = ri->name;
        ut64 value = rz_reg_get_value(core->dbg->reg, ri);
        desc.value = QString::number(value);
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
    return rz_reg_get_name_by_type(core->dbg->reg, registerRole.toUtf8().constData());
}

RVA CutterCore::getProgramCounterValue()
{
    if (currentlyDebugging) {
        CORE_LOCK();
        return rz_debug_reg_get(core->dbg, "PC");
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
    rz_core_reg_assign_sync(core, core->dbg->reg, reg_sync, regName.toUtf8().constData(), val);
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
                    return (void *)nullptr;
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
    asyncCmd("aei; aeim; aeip", debugTask);

    emit debugTaskStateChanged();

    connect(debugTask.data(), &RizinTask::finished, this, [this]() {
        if (debugTaskDialog) {
            delete debugTaskDialog;
        }
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
                    return (void *)NULL;
                },
                debugTask)) {
        return;
    }
    emit debugTaskStateChanged();

    connect(debugTask.data(), &RizinTask::finished, this, [this, uri]() {
        if (debugTaskDialog) {
            delete debugTaskDialog;
        }
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

    QString attach_command = currentlyOpenFile.isEmpty() ? "o" : "oodf";
    // attach to process with dbg plugin
    asyncCmd("e cfg.debug=true;" + attach_command + " dbg://" + QString::number(pid), debugTask);

    emit debugTaskStateChanged();

    connect(debugTask.data(), &RizinTask::finished, this, [this, pid]() {
        if (debugTaskDialog) {
            delete debugTaskDialog;
        }
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

    if (currentlyEmulating) {
        cmdEsil("aeim-; aei-; wcr; .ar-; aets-");
        currentlyEmulating = false;
    } else if (currentlyAttachedToPID != -1) {
        // Use cmd because cmdRaw would not work with command concatenation
        cmd(QString("dp- %1; o %2; .ar-")
                    .arg(QString::number(currentlyAttachedToPID), currentlyOpenFile));
        currentlyAttachedToPID = -1;
    } else {
        QString ptraceFiles = "";
        // close ptrace file descriptors left open
        RzCoreLocked core(Core());
        RzList *descs = rz_id_storage_list(core->io->files);
        RzListIter *it;
        RzIODesc *desc;
        CutterRzListForeach (descs, it, RzIODesc, desc) {
            QString URI = QString(desc->uri);
            if (URI.contains("ptrace")) {
                ptraceFiles += "o-" + QString::number(desc->fd) + ";";
            }
        }
        // Use cmd because cmdRaw would not work with command concatenation
        cmd("doc" + ptraceFiles);
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
        if (!asyncCmdEsil("aec", debugTask)) {
            return;
        }
    } else {
        if (!asyncTask(
                    [](RzCore *core) {
                        rz_debug_continue(core->dbg);
                        return (void *)NULL;
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
        if (!asyncCmdEsil("aecb", debugTask)) {
            return;
        }
    } else {
        if (!asyncTask(
                    [](RzCore *core) {
                        rz_debug_continue_back(core->dbg);
                        return (void *)NULL;
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
        if (!asyncCmdEsil("aecu " + QString::number(offset), debugTask)) {
            return;
        }
    } else {
        if (!asyncTask(
                    [=](RzCore *core) {
                        rz_core_debug_continue_until(core, offset, offset);
                        return (void *)NULL;
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
        if (!asyncCmdEsil("aecc", debugTask)) {
            return;
        }
    } else {
        if (!asyncTask(
                    [](RzCore *core) {
                        rz_core_debug_step_one(core, 0);
                        return (void *)NULL;
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
        if (!asyncCmdEsil("aecs", debugTask)) {
            return;
        }
    } else {
        if (!asyncCmd("dcs", debugTask)) {
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
        if (!asyncCmdEsil("aes", debugTask)) {
            return;
        }
    } else {
        if (!asyncTask(
                    [](RzCore *core) {
                        rz_core_debug_step_one(core, 1);
                        return (void *)NULL;
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
        if (!asyncCmdEsil("aeso", debugTask)) {
            return;
        }
    } else {
        if (!asyncCmd("dso", debugTask)) {
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

void CutterCore::stepOutDebug()
{
    if (!currentlyDebugging) {
        return;
    }

    emit debugTaskStateChanged();
    if (!asyncCmd("dsf", debugTask)) {
        return;
    }

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
        if (!asyncCmdEsil("aesb", debugTask)) {
            return;
        }
    } else {
        if (!asyncCmd("dsb", debugTask)) {
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

QStringList CutterCore::getDebugPlugins()
{
    QStringList plugins;

    for (CutterJson pluginObject : cmdj("dLj")) {
        QString plugin = pluginObject[RJsonKey::name].toString();

        plugins << plugin;
    }
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
        if (!asyncCmdEsil("aets+", debugTask)) {
            return;
        }
    } else {
        if (!asyncTask(
                    [](RzCore *core) {
                        core->dbg->session = rz_debug_session_new();
                        rz_debug_add_checkpoint(core->dbg);
                        return (void *)NULL;
                    },
                    debugTask)) {
            return;
        }
    }
    emit debugTaskStateChanged();

    connect(debugTask.data(), &RizinTask::finished, this, [this]() {
        if (debugTaskDialog) {
            delete debugTaskDialog;
        }
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
        if (!asyncCmdEsil("aets-", debugTask)) {
            return;
        }
    } else {
        if (!asyncTask(
                    [](RzCore *core) {
                        rz_debug_session_free(core->dbg->session);
                        core->dbg->session = NULL;
                        return (void *)NULL;
                    },
                    debugTask)) {
            return;
        }
    }
    emit debugTaskStateChanged();

    connect(debugTask.data(), &RizinTask::finished, this, [this]() {
        if (debugTaskDialog) {
            delete debugTaskDialog;
        }
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
    cmdRaw(QString("dbs %1").arg(addr));
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
    breakpoint = rz_debug_bp_add(core->dbg, address, (config.hw && watchpoint_prot == 0),
                                 watchpoint_prot, watchpoint_prot, module, config.moduleDelta);
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
    if (enabled) {
        cmdRaw(QString("dbite %1").arg(index));
    } else {
        cmdRaw(QString("dbitd %1").arg(index));
    }
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
    int index = breakpointIndexAt(addr);
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

CutterJson CutterCore::getBacktrace()
{
    return cmdj("dbtj");
}

QList<ProcessDescription> CutterCore::getProcessThreads(int pid = -1)
{
    CORE_LOCK();
    RzList *list = rz_debug_pids(core->dbg, pid != -1 ? pid : core->dbg->pid);
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

QList<ProcessDescription> CutterCore::getAllProcesses()
{
    return getProcessThreads(0);
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

void CutterCore::getOpcodes()
{
    this->opcodes = cmdList("?O");
    this->regs = cmdList("drp~[1]");
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
    RzListIter *it;
    QStringList ret;

    RzAsmPlugin *ap;
    CutterRzListForeach (core->rasm->plugins, it, RzAsmPlugin, ap) {
        ret << ap->name;
    }

    return ret;
}

QStringList CutterCore::getAnalysisPluginNames()
{
    CORE_LOCK();
    RzListIter *it;
    QStringList ret;

    RzAnalysisPlugin *ap;
    CutterRzListForeach (core->analysis->plugins, it, RzAnalysisPlugin, ap) {
        ret << ap->name;
    }

    return ret;
}

QList<RzBinPluginDescription> CutterCore::getBinPluginDescriptions(bool bin, bool xtr)
{
    CORE_LOCK();
    QList<RzBinPluginDescription> ret;
    RzListIter *it;
    if (bin) {
        RzBinPlugin *bp;
        CutterRzListForeach (core->bin->plugins, it, RzBinPlugin, bp) {
            RzBinPluginDescription desc;
            desc.name = bp->name ? bp->name : "";
            desc.description = bp->desc ? bp->desc : "";
            desc.license = bp->license ? bp->license : "";
            desc.type = "bin";
            ret.append(desc);
        }
    }
    if (xtr) {
        RzBinXtrPlugin *bx;
        CutterRzListForeach (core->bin->binxtrs, it, RzBinXtrPlugin, bx) {
            RzBinPluginDescription desc;
            desc.name = bx->name ? bx->name : "";
            desc.description = bx->desc ? bx->desc : "";
            desc.license = bx->license ? bx->license : "";
            desc.type = "xtr";
            ret.append(desc);
        }
    }
    return ret;
}

QList<RzIOPluginDescription> CutterCore::getRIOPluginDescriptions()
{
    CORE_LOCK();
    QList<RzIOPluginDescription> ret;
    RzListIter *it;
    RzIOPlugin *p;
    CutterRzListForeach (core->io->plugins, it, RzIOPlugin, p) {
        RzIOPluginDescription desc;
        desc.name = p->name ? p->name : "";
        desc.description = p->desc ? p->desc : "";
        desc.license = p->license ? p->license : "";
        desc.permissions = QString("r") + (p->write ? "w" : "_") + (p->isdbg ? "d" : "_");
        if (p->uris) {
            desc.uris = QString::fromUtf8(p->uris).split(",");
        }
        ret.append(desc);
    }
    return ret;
}

QList<RzCorePluginDescription> CutterCore::getRCorePluginDescriptions()
{
    CORE_LOCK();
    QList<RzCorePluginDescription> ret;
    RzListIter *it;
    RzCorePlugin *p;
    CutterRzListForeach (core->plugins, it, RzCorePlugin, p) {
        RzCorePluginDescription desc;
        desc.name = p->name ? p->name : "";
        desc.description = p->desc ? p->desc : "";
        desc.license = p->license ? p->license : "";
        ret.append(desc);
    }
    return ret;
}

QList<RzAsmPluginDescription> CutterCore::getRAsmPluginDescriptions()
{
    CORE_LOCK();
    RzListIter *it;
    QList<RzAsmPluginDescription> ret;

    RzAsmPlugin *ap;
    CutterRzListForeach (core->rasm->plugins, it, RzAsmPlugin, ap) {
        RzAsmPluginDescription plugin;

        plugin.name = ap->name;
        plugin.architecture = ap->arch;
        plugin.author = ap->author;
        plugin.version = ap->version;
        plugin.cpus = ap->cpus;
        plugin.description = ap->desc;
        plugin.license = ap->license;

        ret << plugin;
    }

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
        function.nargs = rz_analysis_var_count(core->analysis, fcn, 'b', 1)
                + rz_analysis_var_count(core->analysis, fcn, 'r', 1)
                + rz_analysis_var_count(core->analysis, fcn, 's', 1);
        function.nlocals = rz_analysis_var_count(core->analysis, fcn, 'b', 0)
                + rz_analysis_var_count(core->analysis, fcn, 'r', 0)
                + rz_analysis_var_count(core->analysis, fcn, 's', 0);
        function.nbbs = rz_list_length(fcn->bbs);
        function.calltype = fcn->cc ? QString::fromUtf8(fcn->cc) : QString();
        function.name = fcn->name ? QString::fromUtf8(fcn->name) : QString();
        function.edges = rz_analysis_function_count_edges(fcn, nullptr);
        function.stackframe = fcn->maxstack;
        funcList.append(function);
    }

    return funcList;
}

QList<ImportDescription> CutterCore::getAllImports()
{
    CORE_LOCK();
    QList<ImportDescription> ret;

    for (CutterJson importObject : cmdj("iij")) {
        ImportDescription import;

        import.plt = importObject[RJsonKey::plt].toRVA();
        import.ordinal = importObject[RJsonKey::ordinal].toSt64();
        import.bind = importObject[RJsonKey::bind].toString();
        import.type = importObject[RJsonKey::type].toString();
        import.libname = importObject[RJsonKey::libname].toString();
        import.name = importObject[RJsonKey::name].toString();

        ret << import;
    }

    return ret;
}

QList<ExportDescription> CutterCore::getAllExports()
{
    CORE_LOCK();
    QList<ExportDescription> ret;

    for (CutterJson exportObject : cmdj("iEj")) {
        ExportDescription exp;

        exp.vaddr = exportObject[RJsonKey::vaddr].toRVA();
        exp.paddr = exportObject[RJsonKey::paddr].toRVA();
        exp.size = exportObject[RJsonKey::size].toRVA();
        exp.type = exportObject[RJsonKey::type].toString();
        exp.name = exportObject[RJsonKey::name].toString();
        exp.flag_name = exportObject[RJsonKey::flagname].toString();

        ret << exp;
    }

    return ret;
}

QList<SymbolDescription> CutterCore::getAllSymbols()
{
    CORE_LOCK();
    RzListIter *it;

    QList<SymbolDescription> ret;

    RzBinSymbol *bs;
    if (core && core->bin && core->bin->cur && core->bin->cur->o) {
        CutterRzListForeach (core->bin->cur->o->symbols, it, RzBinSymbol, bs) {
            QString type = QString(bs->bind) + " " + QString(bs->type);
            SymbolDescription symbol;
            symbol.vaddr = bs->vaddr;
            symbol.name = QString(bs->name);
            symbol.bind = QString(bs->bind);
            symbol.type = QString(bs->type);
            ret << symbol;
        }

        /* list entrypoints as symbols too */
        int n = 0;
        RzBinAddr *entry;
        CutterRzListForeach (core->bin->cur->o->entries, it, RzBinAddr, entry) {
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
    QList<HeaderDescription> ret;

    for (CutterJson headerObject : cmdj("ihj")) {
        HeaderDescription header;

        header.vaddr = headerObject[RJsonKey::vaddr].toRVA();
        header.paddr = headerObject[RJsonKey::paddr].toRVA();
        header.value = headerObject[RJsonKey::comment].toString();
        header.name = headerObject[RJsonKey::name].toString();

        ret << header;
    }

    return ret;
}

static void sigdb_insert_element_into_qlist(RzList *list, QList<FlirtDescription> &sigdb)
{
    void *ptr = NULL;
    RzListIter *iter = NULL;
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
}

QList<FlirtDescription> CutterCore::getSignaturesDB()
{
    CORE_LOCK();
    QList<FlirtDescription> sigdb;
    RzList *list = nullptr;

    char *system_sigdb = rz_path_system(RZ_SIGDB);
    if (RZ_STR_ISNOTEMPTY(system_sigdb) && rz_file_is_directory(system_sigdb)) {
        list = rz_sign_sigdb_load_database(system_sigdb, true);
        sigdb_insert_element_into_qlist(list, sigdb);
    }
    free(system_sigdb);

    const char *sigdb_path = rz_config_get(core->config, "flirt.sigdb.path");
    if (RZ_STR_ISEMPTY(sigdb_path)) {
        return sigdb;
    }

    list = rz_sign_sigdb_load_database(sigdb_path, true);
    sigdb_insert_element_into_qlist(list, sigdb);

    return sigdb;
}

QList<CommentDescription> CutterCore::getAllComments(const QString &filterType)
{
    CORE_LOCK();
    QList<CommentDescription> ret;

    for (CutterJson commentObject : cmdj("CClj")) {
        QString type = commentObject[RJsonKey::type].toString();
        if (type != filterType)
            continue;

        CommentDescription comment;
        comment.offset = commentObject[RJsonKey::offset].toRVA();
        comment.name = commentObject[RJsonKey::name].toString();

        ret << comment;
    }
    return ret;
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
    return parseStringsJson(cmdjTask("izzj"));
}

QList<StringDescription> CutterCore::parseStringsJson(const CutterJson &doc)
{
    QList<StringDescription> ret;

    for (CutterJson value : doc) {
        StringDescription string;

        string.string = value[RJsonKey::string].toString();
        string.vaddr = value[RJsonKey::vaddr].toRVA();
        string.type = value[RJsonKey::type].toString();
        string.size = value[RJsonKey::size].toUt64();
        string.length = value[RJsonKey::length].toUt64();
        string.section = value[RJsonKey::section].toString();

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

    RzList *sects = rz_bin_object_get_sections(o);
    if (!sects) {
        return sections;
    }
    RzList *hashnames = rz_list_newf(free);
    if (!hashnames) {
        return sections;
    }
    rz_list_push(hashnames, rz_str_new("entropy"));
    RzListIter *it;
    RzBinSection *sect;
    CutterRzListForeach (sects, it, RzBinSection, sect) {
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
            HtPP *digests = rz_core_bin_create_digests(core, sect->paddr, sect->size, hashnames);
            if (!digests) {
                continue;
            }
            const char *entropy = (const char *)ht_pp_find(digests, "entropy", NULL);
            section.entropy = rz_str_get(entropy);
            ht_pp_free(digests);
        }
        section.entropy = "";

        sections << section;
    }
    rz_list_free(sects);
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

    RzList *sects = rz_bin_object_get_sections(o);
    if (!sects) {
        return ret;
    }
    RzListIter *it;
    RzBinSection *sect;
    CutterRzListForeach (sects, it, RzBinSection, sect) {
        ret << sect->name;
    }
    return ret;
}

QList<SegmentDescription> CutterCore::getAllSegments()
{
    CORE_LOCK();
    QList<SegmentDescription> ret;

    for (CutterJson segmentObject : cmdj("iSSj")) {
        QString name = segmentObject[RJsonKey::name].toString();
        if (name.isEmpty())
            continue;

        SegmentDescription segment;
        segment.name = name;
        segment.vaddr = segmentObject[RJsonKey::vaddr].toRVA();
        segment.paddr = segmentObject[RJsonKey::paddr].toRVA();
        segment.size = segmentObject[RJsonKey::size].toRVA();
        segment.vsize = segmentObject[RJsonKey::vsize].toRVA();
        segment.perm = segmentObject[RJsonKey::perm].toString();

        ret << segment;
    }
    return ret;
}

QList<EntrypointDescription> CutterCore::getAllEntrypoint()
{
    CORE_LOCK();
    QList<EntrypointDescription> ret;

    for (CutterJson entrypointObject : cmdj("iej")) {
        EntrypointDescription entrypoint;

        entrypoint.vaddr = entrypointObject[RJsonKey::vaddr].toRVA();
        entrypoint.paddr = entrypointObject[RJsonKey::paddr].toRVA();
        entrypoint.baddr = entrypointObject[RJsonKey::baddr].toRVA();
        entrypoint.laddr = entrypointObject[RJsonKey::laddr].toRVA();
        entrypoint.haddr = entrypointObject[RJsonKey::haddr].toRVA();
        entrypoint.type = entrypointObject[RJsonKey::type].toString();

        ret << entrypoint;
    }
    return ret;
}

QList<BinClassDescription> CutterCore::getAllClassesFromBin()
{
    CORE_LOCK();
    QList<BinClassDescription> ret;

    for (CutterJson classObject : cmdj("icj")) {
        BinClassDescription cls;

        cls.name = classObject[RJsonKey::classname].toString();
        cls.addr = classObject[RJsonKey::addr].toRVA();
        cls.index = classObject[RJsonKey::index].toUt64();

        for (CutterJson methObject : classObject[RJsonKey::methods]) {
            BinClassMethodDescription meth;

            meth.name = methObject[RJsonKey::name].toString();
            meth.addr = methObject[RJsonKey::addr].toRVA();

            cls.methods << meth;
        }

        for (CutterJson fieldObject : classObject[RJsonKey::fields]) {
            BinClassFieldDescription field;

            field.name = fieldObject[RJsonKey::name].toString();
            field.addr = fieldObject[RJsonKey::addr].toRVA();

            cls.fields << field;
        }

        ret << cls;
    }
    return ret;
}

QList<BinClassDescription> CutterCore::getAllClassesFromFlags()
{
    static const QRegularExpression classFlagRegExp("^class\\.(.*)$");
    static const QRegularExpression methodFlagRegExp("^method\\.([^\\.]*)\\.(.*)$");

    CORE_LOCK();
    QList<BinClassDescription> ret;
    QMap<QString, BinClassDescription *> classesCache;

    for (const CutterJson flagObject : cmdj("fj@F:classes")) {
        QString flagName = flagObject[RJsonKey::name].toString();

        QRegularExpressionMatch match = classFlagRegExp.match(flagName);
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
            desc->addr = flagObject[RJsonKey::offset].toRVA();
            desc->index = RVA_INVALID;
            continue;
        }

        match = methodFlagRegExp.match(flagName);
        if (match.hasMatch()) {
            QString className = match.captured(1);
            BinClassDescription *classDesc = nullptr;
            auto it = classesCache.find(className);
            if (it == classesCache.end()) {
                // add a new stub class, will be replaced if class flag comes after it
                BinClassDescription cls;
                cls.name = tr("Unknown (%1)").arg(className);
                cls.addr = RVA_INVALID;
                cls.index = 0;
                ret << cls;
                classDesc = &ret.last();
                classesCache[className] = classDesc;
            } else {
                classDesc = it.value();
            }

            BinClassMethodDescription meth;
            meth.name = match.captured(2);
            meth.addr = flagObject[RJsonKey::offset].toRVA();
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

    SdbListPtr l = makeSdbListPtr(rz_analysis_class_get_all(core->analysis, sorted));
    if (!l) {
        return ret;
    }
    ret.reserve(static_cast<int>(l->length));

    SdbListIter *it;
    void *entry;
    ls_foreach(l, it, entry)
    {
        auto kv = reinterpret_cast<SdbKv *>(entry);
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
    analysisMeth.name = rz_str_new(meth.name.toUtf8().constData());
    analysisMeth.real_name = rz_str_new(meth.realName.toUtf8().constData());
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
    QList<ResourcesDescription> resources;

    for (CutterJson resourceObject : cmdj("iRj")) {
        ResourcesDescription res;

        res.name = resourceObject[RJsonKey::name].toString();
        res.vaddr = resourceObject[RJsonKey::vaddr].toRVA();
        res.index = resourceObject[RJsonKey::index].toUt64();
        res.type = resourceObject[RJsonKey::type].toString();
        res.size = resourceObject[RJsonKey::size].toUt64();
        res.lang = resourceObject[RJsonKey::lang].toString();

        resources << res;
    }
    return resources;
}

QList<VTableDescription> CutterCore::getAllVTables()
{
    CORE_LOCK();
    QList<VTableDescription> vtables;

    for (CutterJson vTableObject : cmdj("avj")) {
        VTableDescription res;

        res.addr = vTableObject[RJsonKey::offset].toRVA();

        for (CutterJson methodObject : vTableObject[RJsonKey::methods]) {
            BinClassMethodDescription method;

            method.addr = methodObject[RJsonKey::offset].toRVA();
            method.name = methodObject[RJsonKey::name].toString();

            res.methods << method;
        }

        vtables << res;
    }
    return vtables;
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
        exp.format = rz_type_format(core->analysis->typedb, type->name);
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
    // TODO: use API for `tc` command once available
    QString result = cmd(QString("tc %1").arg(earg));
    free(earg);
    return result;
}

bool CutterCore::isAddressMapped(RVA addr)
{
    // If value returned by "om. @ addr" is empty means that address is not mapped
    return !Core()->cmdRawAt(QString("om."), addr).isEmpty();
}

QList<SearchDescription> CutterCore::getAllSearch(QString searchFor, QString space, QString in)
{
    CORE_LOCK();
    QList<SearchDescription> searchRef;

    CutterJson searchArray;
    {
        TempConfig cfg;
        cfg.set("search.in", in);
        searchArray = cmdj(QString("%1 %2").arg(space, searchFor));
    }

    if (space == "/Rj") {
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
    } else {
        for (CutterJson searchObject : searchArray) {
            SearchDescription exp;

            exp.offset = searchObject[RJsonKey::offset].toRVA();
            exp.size = searchObject[RJsonKey::len].toUt64();
            exp.code = searchObject[RJsonKey::code].toString();
            exp.data = searchObject[RJsonKey::data].toString();

            searchRef << exp;
        }
    }
    return searchRef;
}

BlockStatistics CutterCore::getBlockStatistics(unsigned int blocksCount)
{
    BlockStatistics blockStats;
    if (blocksCount == 0) {
        blockStats.from = blockStats.to = blockStats.blocksize = 0;
        return blockStats;
    }

    CutterJson statsObj;

    // User TempConfig here to set the search boundaries to all sections. This makes sure
    // that the Visual Navbar will show all the relevant addresses.
    {
        TempConfig tempConfig;
        tempConfig.set("search.in", "bin.sections");
        statsObj = cmdj("p-j " + QString::number(blocksCount));
    }

    blockStats.from = statsObj[RJsonKey::from].toRVA();
    blockStats.to = statsObj[RJsonKey::to].toRVA();
    blockStats.blocksize = statsObj[RJsonKey::blocksize].toRVA();

    for (CutterJson blockObj : statsObj[RJsonKey::blocks]) {
        BlockDescription block;

        block.addr = blockObj[RJsonKey::offset].toRVA();
        block.size = blockObj[RJsonKey::size].toRVA();
        block.flags = blockObj[RJsonKey::flags].toSt64();
        block.functions = blockObj[RJsonKey::functions].toSt64();
        block.inFunctions = blockObj[RJsonKey::in_functions].toSt64();
        block.comments = blockObj[RJsonKey::comments].toSt64();
        block.symbols = blockObj[RJsonKey::symbols].toSt64();
        block.strings = blockObj[RJsonKey::strings].toSt64();

        block.rwx = 0;
        QString rwxStr = blockObj[RJsonKey::rwx].toString();
        if (rwxStr.length() == 3) {
            if (rwxStr[0] == 'r') {
                block.rwx |= (1 << 0);
            }
            if (rwxStr[1] == 'w') {
                block.rwx |= (1 << 1);
            }
            if (rwxStr[2] == 'x') {
                block.rwx |= (1 << 2);
            }
        }

        blockStats.blocks << block;
    }

    return blockStats;
}

QList<XrefDescription> CutterCore::getXRefsForVariable(QString variableName, bool findWrites,
                                                       RVA offset)
{
    QList<XrefDescription> xrefList = QList<XrefDescription>();
    for (CutterJson xrefObject : cmdjAt(findWrites ? "afvWj" : "afvRj", offset)) {
        QString name = xrefObject[RJsonKey::name].toString();
        if (name == variableName) {
            for (CutterJson address : xrefObject[RJsonKey::addrs]) {
                XrefDescription xref;
                RVA addr = address.toRVA();
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
        xd.to_str = Core()->cmdRaw(QString("fd %1").arg(xd.to)).trimmed();

        xrefList << xd;
    }
    rz_list_free(xrefs);
    return xrefList;
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
    auto r = cmdj(QString("fdj @ ") + QString::number(offset));
    QString name = r["name"].toString();
    if (flagOffsetOut) {
        auto offsetValue = r["offset"];
        *flagOffsetOut = offsetValue.valid() ? offsetValue.toRVA() : offset;
    }
    return name;
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
    CutterJson array = cmdj(QString("pdJ ") + QString::number(lines) + QString(" @ ")
                            + QString::number(offset));
    QList<DisassemblyLine> r;

    for (CutterJson object : array) {
        DisassemblyLine line;
        line.offset = object[RJsonKey::offset].toRVA();
        line.text = ansiEscapeToHtml(object[RJsonKey::text].toString());
        line.arrow = object[RJsonKey::arrow].toRVA();
        r << line;
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
    QString command = "px";
    switch (format) {
    case HexdumpFormats::Normal:
        break;
    case HexdumpFormats::Half:
        command += "h";
        break;
    case HexdumpFormats::Word:
        command += "w";
        break;
    case HexdumpFormats::Quad:
        command += "q";
        break;
    case HexdumpFormats::Signed:
        command += "d";
        break;
    case HexdumpFormats::Octal:
        command += "o";
        break;
    }

    return cmdRawAt(QString("%1 %2").arg(command).arg(size), address);
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
    QByteArray hex;
    hex.resize(bytes.length() * 2);
    rz_hex_bin2str(reinterpret_cast<const ut8 *>(bytes.constData()), bytes.size(), hex.data());
    return QString::fromUtf8(hex);
}

void CutterCore::loadScript(const QString &scriptname)
{
    {
        CORE_LOCK();
        rz_core_cmd_file(core, scriptname.toUtf8().constData());
    }
    triggerRefreshAll();
}

QString CutterCore::getRizinVersionReadable()
{
    return QString("%1 (%2)").arg(QString::fromUtf8(RZ_VERSION),
                                  QString::fromUtf8(RZ_GITTIP).left(7));
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
        { "rz_analysis", &rz_analysis_version },
        { "rz_lib", &rz_lib_version },
        { "rz_egg", &rz_egg_version },
        { "rz_asm", &rz_asm_version },
        { "rz_bin", &rz_bin_version },
        { "rz_cons", &rz_cons_version },
        { "rz_flag", &rz_flag_version },
        { "rz_core", &rz_core_version },
        { "rz_crypto", &rz_crypto_version },
        { "rz_bp", &rz_bp_version },
        { "rz_debug", &rz_debug_version },
        { "rz_msg_digest", &rz_msg_digest_version },
        { "rz_io", &rz_io_version },
#if !USE_LIB_MAGIC
        { "rz_magic", &rz_magic_version },
#endif
        { "rz_parse", &rz_parse_version },
        { "rz_reg", &rz_reg_version },
        { "rz_sign", &rz_sign_version },
        { "rz_search", &rz_search_version },
        { "rz_syscall", &rz_syscall_version },
        { "rz_util", &rz_util_version },
        /* ... */
        { NULL, NULL }
    };
    versionInfo.append(QString("%1 rz\n").arg(getRizinVersionReadable()));
    for (i = 0; vcs[i].name; i++) {
        struct vcs_t *v = &vcs[i];
        const char *name = v->callback();
        versionInfo.append(QString("%1 %2\n").arg(name, v->name));
    }
    return versionInfo;
}

QList<QString> CutterCore::getColorThemes()
{
    QList<QString> r;
    for (CutterJson s : cmdj("ecoj")) {
        r << s.toString();
    }
    return r;
}

QString CutterCore::ansiEscapeToHtml(const QString &text)
{
    int len;
    char *html = rz_cons_html_filter(text.toUtf8().constData(), &len);
    if (!html) {
        return QString();
    }
    QString r = QString::fromUtf8(html, len);
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
    if (!isWriteModeEnabled()) {
        cmdRaw("oo+");
        rz_io_cache_commit(core->io, 0, UT64_MAX);
        rz_core_block_read(core);
        cmdRaw("oo");
    } else {
        rz_io_cache_commit(core->io, 0, UT64_MAX);
        rz_core_block_read(core);
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

    // Change from read-only to write-mode
    if (enabled && !writeModeState) {
        cmdRaw("oo+");
        // Change from write-mode to read-only
    } else {
        cmdRaw("oo");
    }
    // Disable cache mode because we specifically set write or
    // read-only modes.
    setIOCache(false);
    writeModeChanged(enabled);
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
        qWarning() << "Can't read data" << addr << len;
        array.fill(0xff);
    }

    return array;
}
