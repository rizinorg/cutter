#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QDir>
#include <QCoreApplication>

#include "common/TempConfig.h"
#include "common/Configuration.h"
#include "common/AsyncTask.h"
#include "common/R2Task.h"
#include "common/Json.h"
#include "core/Cutter.h"
#include "r_asm.h"
#include "sdb.h"

Q_GLOBAL_STATIC(CutterCore, uniqueInstance)

#define R_JSON_KEY(name) static const QString name = QStringLiteral(#name)

namespace RJsonKey {
    R_JSON_KEY(addr);
    R_JSON_KEY(addr_end);
    R_JSON_KEY(baddr);
    R_JSON_KEY(bind);
    R_JSON_KEY(blocks);
    R_JSON_KEY(blocksize);
    R_JSON_KEY(bytes);
    R_JSON_KEY(calltype);
    R_JSON_KEY(cc);
    R_JSON_KEY(classname);
    R_JSON_KEY(code);
    R_JSON_KEY(comment);
    R_JSON_KEY(comments);
    R_JSON_KEY(cost);
    R_JSON_KEY(data);
    R_JSON_KEY(description);
    R_JSON_KEY(ebbs);
    R_JSON_KEY(edges);
    R_JSON_KEY(enabled);
    R_JSON_KEY(entropy);
    R_JSON_KEY(fcn_addr);
    R_JSON_KEY(fcn_name);
    R_JSON_KEY(fields);
    R_JSON_KEY(file);
    R_JSON_KEY(flags);
    R_JSON_KEY(flagname);
    R_JSON_KEY(format);
    R_JSON_KEY(from);
    R_JSON_KEY(functions);
    R_JSON_KEY(graph);
    R_JSON_KEY(haddr);
    R_JSON_KEY(hw);
    R_JSON_KEY(in_functions);
    R_JSON_KEY(index);
    R_JSON_KEY(jump);
    R_JSON_KEY(laddr);
    R_JSON_KEY(lang);
    R_JSON_KEY(len);
    R_JSON_KEY(length);
    R_JSON_KEY(license);
    R_JSON_KEY(methods);
    R_JSON_KEY(name);
    R_JSON_KEY(nargs);
    R_JSON_KEY(nbbs);
    R_JSON_KEY(nlocals);
    R_JSON_KEY(offset);
    R_JSON_KEY(opcode);
    R_JSON_KEY(opcodes);
    R_JSON_KEY(ordinal);
    R_JSON_KEY(outdegree);
    R_JSON_KEY(paddr);
    R_JSON_KEY(path);
    R_JSON_KEY(perm);
    R_JSON_KEY(pid);
    R_JSON_KEY(plt);
    R_JSON_KEY(prot);
    R_JSON_KEY(ref);
    R_JSON_KEY(refs);
    R_JSON_KEY(reg);
    R_JSON_KEY(rwx);
    R_JSON_KEY(section);
    R_JSON_KEY(sections);
    R_JSON_KEY(size);
    R_JSON_KEY(stackframe);
    R_JSON_KEY(status);
    R_JSON_KEY(string);
    R_JSON_KEY(strings);
    R_JSON_KEY(symbols);
    R_JSON_KEY(text);
    R_JSON_KEY(to);
    R_JSON_KEY(trace);
    R_JSON_KEY(type);
    R_JSON_KEY(uid);
    R_JSON_KEY(vaddr);
    R_JSON_KEY(value);
    R_JSON_KEY(vsize);
}

#undef R_JSON_KEY

RCoreLocked::RCoreLocked(RCore *core)
    : core(core)
{
    r_th_lock_enter(core->lock);
}

RCoreLocked::RCoreLocked(RCoreLocked &&o)
    : core(o.core)
{
    o.core = nullptr;
}

RCoreLocked::~RCoreLocked()
{
    r_th_lock_leave(core->lock);
}

RCoreLocked::operator RCore *() const
{
    return core;
}

RCore *RCoreLocked::operator->() const
{
    return core;
}

RCoreLocked CutterCore::core() const
{
    return RCoreLocked(this->core_);
}

#define CORE_LOCK() RCoreLocked core_lock__(this->core_)

static void cutterREventCallback(REvent *, int type, void *user, void *data)
{
    auto core = reinterpret_cast<CutterCore *>(user);
    core->handleREvent(type, data);
}

CutterCore::CutterCore(QObject *parent) :
    QObject(parent)
{
}


CutterCore *CutterCore::instance()
{
    return uniqueInstance;
}

void CutterCore::initialize()
{
    r_cons_new();  // initialize console
    core_ = r_core_new();

    r_event_hook(core_->anal->ev, R_EVENT_ALL, cutterREventCallback, this);

#if defined(APPIMAGE) || defined(MACOS_R2_BUNDLED)
    auto prefix = QDir(QCoreApplication::applicationDirPath());
#ifdef APPIMAGE
    // Executable is in appdir/bin
    prefix.cdUp();
    qInfo() << "Setting r2 prefix =" << prefix.absolutePath() << " for AppImage.";
#else // MACOS_R2_BUNDLED
    // Executable is in Contents/MacOS, prefix is Contents/Resources/r2
    prefix.cdUp();
    prefix.cd("Resources");
    prefix.cd("r2");
    qInfo() << "Setting r2 prefix =" << prefix.absolutePath() << " for macOS Application Bundle.";
#endif
    setConfig("dir.prefix", prefix.absolutePath());
#endif

    r_core_loadlibs(this->core_, R_CORE_LOADLIBS_ALL, NULL);
    // IMPLICIT r_bin_iobind (core_->bin, core_->io);

    // Otherwise r2 may ask the user for input and Cutter would freeze
    setConfig("scr.interactive", false);

    // Initialize graph node highlighter
    bbHighlighter = new BasicBlockHighlighter();

    // Initialize Async tasks manager
    asyncTaskManager = new AsyncTaskManager(this);
}

QList<QString> CutterCore::sdbList(QString path)
{
    CORE_LOCK();
    QList<QString> list = QList<QString>();
    Sdb *root = sdb_ns_path(core_->sdb, path.toUtf8().constData(), 0);
    if (root) {
        void *vsi;
        ls_iter_t *iter;
        ls_foreach(root->ns, iter, vsi) {
            SdbNs *nsi = (SdbNs *)vsi;
            list << nsi->name;
        }
    }
    return list;
}

QList<QString> CutterCore::sdbListKeys(QString path)
{
    CORE_LOCK();
    QList<QString> list = QList<QString>();
    Sdb *root = sdb_ns_path(core_->sdb, path.toUtf8().constData(), 0);
    if (root) {
        void *vsi;
        ls_iter_t *iter;
        SdbList *l = sdb_foreach_list(root, false);
        ls_foreach(l, iter, vsi) {
            SdbKv *nsi = (SdbKv *)vsi;
            list << reinterpret_cast<char *>(nsi->base.key);
        }
    }
    return list;
}

QString CutterCore::sdbGet(QString path, QString key)
{
    CORE_LOCK();
    Sdb *db = sdb_ns_path(core_->sdb, path.toUtf8().constData(), 0);
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
    Sdb *db = sdb_ns_path(core_->sdb, path.toUtf8().constData(), 1);
    if (!db) return false;
    return sdb_set(db, key.toUtf8().constData(), val.toUtf8().constData(), 0);
}

CutterCore::~CutterCore()
{
    delete bbHighlighter;
    r_core_free(this->core_);
    r_cons_free();
}

QString CutterCore::sanitizeStringForCommand(QString s)
{
    static const QRegExp regexp(";|@");
    return s.replace(regexp, QStringLiteral("_"));
}

/**
 * @brief CutterCore::cmd send a command to radare2
 * @param str the command you want to execute
 * Note that if you want to seek to an address, you should use CutterCore::seek
 * @return command output
 */
QString CutterCore::cmd(const char *str)
{
    CORE_LOCK();

    RVA offset = core_->offset;
    r_core_task_sync_begin(core_);
    char *res = r_core_cmd_str(this->core_, str);
    r_core_task_sync_end(core_);
    QString o = QString(res ? res : "");
    r_mem_free(res);
    if (offset != core_->offset) {
        updateSeek();
    }
    return o;
}

QString CutterCore::cmdRaw(const QString &str)
{
    QString cmdStr = str;
    cmdStr.replace('\"', QStringLiteral("\\\""));
    return cmd(cmdStr.prepend('\"').append('\"'));
}

QJsonDocument CutterCore::cmdj(const char *str)
{
    CORE_LOCK();

    r_core_task_sync_begin(core_);
    char *res = r_core_cmd_str(this->core_, str);
    r_core_task_sync_end(core_);
    QJsonDocument doc = parseJson(res, str);
    r_mem_free(res);

    return doc;
}

QString CutterCore::cmdTask(const QString &str)
{
    R2Task task(str);
    task.startTask();
    task.joinTask();
    return task.getResult();
}

QJsonDocument CutterCore::cmdjTask(const QString &str)
{
    R2Task task(str);
    task.startTask();
    task.joinTask();
    return parseJson(task.getResultRaw(), str);
}

QJsonDocument CutterCore::parseJson(const char *res, const char *cmd)
{
    QByteArray json(res);

    if (json.isEmpty()) {
        return QJsonDocument();
    }

    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(json, &jsonError);

    if (jsonError.error != QJsonParseError::NoError) {
        if (cmd) {
            eprintf("Failed to parse JSON for command \"%s\": %s\n", cmd,
                    jsonError.errorString().toLocal8Bit().constData());
        } else {
            eprintf("Failed to parse JSON: %s\n", jsonError.errorString().toLocal8Bit().constData());
        }
        eprintf("%s\n", json.constData());
    }

    return doc;
}

QStringList CutterCore::autocomplete(const QString &cmd, RLinePromptType promptType, size_t limit)
{
    RLineBuffer buf;
    int c = snprintf(buf.data, sizeof(buf.data), "%s", cmd.toUtf8().constData());
    if (c < 0) {
        return {};
    }
    buf.index = buf.length = std::min((int)(sizeof(buf.data) - 1), c);

    RLineCompletion completion;
    r_line_completion_init(&completion, limit);
    r_core_autocomplete(core(), &completion, &buf, promptType);

    QStringList r;
    r.reserve(r_pvector_len(&completion.args));
    for (size_t i = 0; i< r_pvector_len(&completion.args); i++) {
        r.push_back(QString::fromUtf8(reinterpret_cast<const char *>(r_pvector_at(&completion.args, i))));
    }

    r_line_completion_fini(&completion);
    return r;
}

/**
 * @brief CutterCore::loadFile
 * Load initial file. TODO Maybe use the "o" commands?
 * @param path File path
 * @param baddr Base (RBin) address
 * @param mapaddr Map address
 * @param perms
 * @param va
 * @param loadbin Load RBin information
 * @param forceBinPlugin
 * @return
 */
bool CutterCore::loadFile(QString path, ut64 baddr, ut64 mapaddr, int perms, int va,
                          bool loadbin, const QString &forceBinPlugin)
{
    CORE_LOCK();
    RCoreFile *f;
    r_config_set_i(core_->config, "io.va", va);

    f = r_core_file_open(core_, path.toUtf8().constData(), perms, mapaddr);
    if (!f) {
        eprintf("r_core_file_open failed\n");
        return false;
    }

    if (!forceBinPlugin.isNull()) {
        r_bin_force_plugin(r_core_get_bin(core_), forceBinPlugin.toUtf8().constData());
    }

    if (loadbin && va) {
        if (!r_core_bin_load(core_, path.toUtf8().constData(), baddr)) {
            eprintf("CANNOT GET RBIN INFO\n");
        }

#if HAVE_MULTIPLE_RBIN_FILES_INSIDE_SELECT_WHICH_ONE
        if (!r_core_file_open(core, path.toUtf8(), R_IO_READ | (rw ? R_IO_WRITE : 0, mapaddr))) {
            eprintf("Cannot open file\n");
        } else {
            // load RBin information
            // XXX only for sub-bins
            r_core_bin_load(core, path.toUtf8(), baddr);
            r_bin_select_idx(core_->bin, NULL, idx);
        }
#endif
    } else {
        // Not loading RBin info coz va = false
    }

    auto iod = core_->io ? core_->io->desc : NULL;
    auto debug = core_->file && iod && (core_->file->fd == iod->fd) && iod->plugin && \
                 iod->plugin->isdbg;

    if (!debug && r_flag_get (core_->flags, "entry0")) {
        r_core_cmd0 (core_, "s entry0");
    }

    if (perms & R_PERM_W) {
        r_core_cmd0 (core_, "omfg+w");
    }

    ut64 hashLimit = getConfigut64("cfg.hashlimit");
    r_bin_file_hash(core_->bin, hashLimit, path.toUtf8().constData(), NULL);

    fflush(stdout);
    return true;
}

bool CutterCore::tryFile(QString path, bool rw)
{
    CORE_LOCK();
    RCoreFile *cf;
    int flags = R_PERM_R;
    if (rw) flags = R_PERM_RW;
    cf = r_core_file_open(this->core_, path.toUtf8().constData(), flags, 0LL);
    if (!cf) {
        return false;
    }

    r_core_file_close (this->core_, cf);

    return true;
}

/**
 * @brief Opens a file using r2 API
 * @param path Path to file
 * @param mapaddr Map Address
 * @return bool
 */
bool CutterCore::openFile(QString path, RVA mapaddr)
{
    CORE_LOCK();
    RVA addr = mapaddr != RVA_INVALID ? mapaddr : 0;
    ut64 baddr = Core()->getFileInfo().object()["bin"].toObject()["baddr"].toVariant().toULongLong();
    if (r_core_file_open(core_, path.toUtf8().constData(), R_PERM_RX, addr)) {
        r_core_bin_load(core_, path.toUtf8().constData(), baddr);
    } else {
        return false;
    }
    return true;
}

void CutterCore::renameFunction(const QString &oldName, const QString &newName)
{
    cmdRaw("afn " + newName + " " + oldName);
    emit functionRenamed(oldName, newName);
}

void CutterCore::delFunction(RVA addr)
{
    cmd("af- " + RAddressString(addr));
    emit functionsChanged();
}

void CutterCore::renameFlag(QString old_name, QString new_name)
{
    cmdRaw("fr " + old_name + " " + new_name);
    emit flagsChanged();
}

void CutterCore::delFlag(RVA addr)
{
    cmd("f-@" + RAddressString(addr));
    emit flagsChanged();
}

void CutterCore::delFlag(const QString &name)
{
    cmdRaw("f-" + name);
    emit flagsChanged();
}

QString CutterCore::getInstructionBytes(RVA addr)
{
    return cmdj("aoj @ " + RAddressString(addr)).array().first().toObject()[RJsonKey::bytes].toString();
}

QString CutterCore::getInstructionOpcode(RVA addr)
{
    return cmdj("aoj @ " + RAddressString(addr)).array().first().toObject()[RJsonKey::opcode].toString();
}

void CutterCore::editInstruction(RVA addr, const QString &inst)
{
    cmd("wa " + inst + " @ " + RAddressString(addr));
    emit instructionChanged(addr);
}

void CutterCore::nopInstruction(RVA addr)
{
    cmd("wao nop @ " + RAddressString(addr));
    emit instructionChanged(addr);
}

void CutterCore::jmpReverse(RVA addr)
{
    cmd("wao recj @ " + RAddressString(addr));
    emit instructionChanged(addr);
}

void CutterCore::editBytes(RVA addr, const QString &bytes)
{
    cmd("wx " + bytes + " @ " + RAddressString(addr));
    emit instructionChanged(addr);
}

void CutterCore::editBytesEndian(RVA addr, const QString &bytes)
{
    cmd("wv " + bytes + " @ " + RAddressString(addr));
    emit stackChanged();
}

void CutterCore::setToCode(RVA addr)
{
    cmd("Cd- @ " + RAddressString(addr));
    emit instructionChanged(addr);
}

void CutterCore::setAsString(RVA addr)
{
    cmd("Cs @ " + RAddressString(addr));
    emit instructionChanged(addr);
}

void CutterCore::setToData(RVA addr, int size, int repeat)
{
    if (size <= 0 || repeat <= 0) {
        return;
    }
    cmd("Cd- @ " + RAddressString(addr));
    cmd(QString::asprintf("Cd %d %d @ %lld", size, repeat, addr));
    emit instructionChanged(addr);
}

int CutterCore::sizeofDataMeta(RVA addr)
{
    bool ok;
    int size = cmd("Cd. @ " + RAddressString(addr)).toInt(&ok);
    return (ok ? size : 0);
}

void CutterCore::setComment(RVA addr, const QString &cmt)
{
    cmd("CCu base64:" + cmt.toLocal8Bit().toBase64() + " @ " + QString::number(addr));
    emit commentsChanged();
}

void CutterCore::delComment(RVA addr)
{
    cmd("CC- @ " + QString::number(addr));
    emit commentsChanged();
}

void CutterCore::setImmediateBase(const QString &r2BaseName, RVA offset)
{
    if (offset == RVA_INVALID) {
        offset = getOffset();
    }

    this->cmd("ahi " + r2BaseName + " @ " + QString::number(offset));
    emit instructionChanged(offset);
}

void CutterCore::setCurrentBits(int bits, RVA offset)
{
    if (offset == RVA_INVALID) {
        offset = getOffset();
    }

    this->cmd("ahb " + QString::number(bits) + " @ " + QString::number(offset));
    emit instructionChanged(offset);
}

void CutterCore::applyStructureOffset(const QString &structureOffset, RVA offset)
{
    if (offset == RVA_INVALID) {
        offset = getOffset();
    }

    this->cmdRaw("ta " + structureOffset + " @ " + QString::number(offset));
    emit instructionChanged(offset);
}

void CutterCore::seek(ut64 offset)
{
    // Slower than using the API, but the API is not complete
    // which means we either have to duplicate code from radare2
    // here, or refactor radare2 API.
    CORE_LOCK();
    if (offset == RVA_INVALID) {
        return;
    }
    cmd(QString("s %1").arg(offset));
    // cmd already does emit seekChanged(core_->offset);
    triggerRaisePrioritizedMemoryWidget();
}

void CutterCore::seek(QString thing)
{
    cmdRaw(QString("s %1").arg(thing));
    triggerRaisePrioritizedMemoryWidget();
}

void CutterCore::seekPrev()
{
    cmd("s-");
    triggerRaisePrioritizedMemoryWidget();
}

void CutterCore::seekNext()
{
    cmd("s+");
    triggerRaisePrioritizedMemoryWidget();
}

void CutterCore::updateSeek()
{
    emit seekChanged(core_->offset);
}

RVA CutterCore::prevOpAddr(RVA startAddr, int count)
{
    CORE_LOCK();
    bool ok;
    RVA offset = cmd("/O " + QString::number(count) + " @ " + QString::number(startAddr)).toULongLong(
                     &ok, 16);
    return ok ? offset : startAddr - count;
}

RVA CutterCore::nextOpAddr(RVA startAddr, int count)
{
    CORE_LOCK();

    QJsonArray array = Core()->cmdj("pdj " + QString::number(count + 1) + "@" + QString::number(
                                        startAddr)).array();
    if (array.isEmpty()) {
        return startAddr + 1;
    }

    QJsonValue instValue = array.last();
    if (!instValue.isObject()) {
        return startAddr + 1;
    }

    bool ok;
    RVA offset = instValue.toObject()[RJsonKey::offset].toVariant().toULongLong(&ok);
    if (!ok) {
        return startAddr + 1;
    }

    return offset;
}

RVA CutterCore::getOffset()
{
    return core_->offset;
}

ut64 CutterCore::math(const QString &expr)
{
    CORE_LOCK();
    return r_num_math(this->core_ ? this->core_->num : NULL, expr.toUtf8().constData());
}

ut64 CutterCore::num(const QString &expr)
{
    CORE_LOCK();
    return r_num_get(this->core_ ? this->core_->num : NULL, expr.toUtf8().constData());
}

QString CutterCore::itoa(ut64 num, int rdx)
{
    return QString::number(num, rdx);
}

void CutterCore::setConfig(const char *k, const QString &v)
{
    CORE_LOCK();
    r_config_set(core_->config, k, v.toUtf8().constData());
}

void CutterCore::setConfig(const char *k, int v)
{
    CORE_LOCK();
    r_config_set_i(core_->config, k, static_cast<ut64>(v));
}

void CutterCore::setConfig(const char *k, bool v)
{
    CORE_LOCK();
    r_config_set_i(core_->config, k, v ? 1 : 0);
}

int CutterCore::getConfigi(const char *k)
{
    CORE_LOCK();
    return static_cast<int>(r_config_get_i(core_->config, k));
}

ut64 CutterCore::getConfigut64(const char *k)
{
    CORE_LOCK();
    return r_config_get_i(core_->config, k);
}

bool CutterCore::getConfigb(const char *k)
{
    CORE_LOCK();
    return r_config_get_i(core_->config, k) != 0;
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
    return QString(r_config_get(core_->config, k));
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
    RAsmCode *ac = r_asm_massemble(core_->assembler, code.toUtf8().constData());
    QByteArray res;
    if (ac && ac->bytes) {
        res = QByteArray(reinterpret_cast<const char *>(ac->bytes), ac->len);
    }
    r_asm_code_free(ac);
    return res;
}

QString CutterCore::disassemble(const QByteArray &data)
{
    CORE_LOCK();
    RAsmCode *ac = r_asm_mdisassemble(core_->assembler, reinterpret_cast<const ut8 *>(data.constData()), data.length());
    QString code;
    if (ac && ac->assembly) {
        code = QString::fromUtf8(ac->assembly);
    }
    r_asm_code_free(ac);
    return code;
}

QString CutterCore::disassembleSingleInstruction(RVA addr)
{
    return cmd("pi 1@" + QString::number(addr)).simplified();
}

RAnalFunction *CutterCore::functionAt(ut64 addr)
{
    CORE_LOCK();
    return r_anal_get_fcn_in(core_->anal, addr, 0);
}

QString CutterCore::cmdFunctionAt(QString addr)
{
    QString ret;
    ret = cmd(QString("fd @ ") + addr + "~[0]");
    return ret.trimmed();
}

QString CutterCore::cmdFunctionAt(RVA addr)
{
    return cmdFunctionAt(QString::number(addr));
}

void CutterCore::cmdEsil(const char *command)
{
    QString res = cmd(command);
    if (res.contains(QStringLiteral("[ESIL] Stopped execution in an invalid instruction"))) {
        msgBox.showMessage("Stopped when attempted to run an invalid instruction. You can disable this in Preferences");
    }
}

QString CutterCore::createFunctionAt(RVA addr, QString name)
{
    static const QRegExp regExp("[^a-zA-Z0-9_]");
    name.remove(regExp);
    QString command = "af " + name + " " + RAddressString(addr);
    QString ret = cmd(command);
    emit functionsChanged();
    return ret;
}

QJsonDocument CutterCore::getRegistersInfo()
{
    return cmdj("aeafj");
}

RVA CutterCore::getOffsetJump(RVA addr)
{
    bool ok;
    RVA value = cmdj("aoj @" + QString::number(
                         addr)).array().first().toObject().value(RJsonKey::jump).toVariant().toULongLong(&ok);

    if (!ok) {
        return RVA_INVALID;
    }

    return value;
}

QString CutterCore::getDecompiledCodePDC(RVA addr)
{
    return cmd("pdc @ " + QString::number(addr));
}

bool CutterCore::getR2DecAvailable()
{
    return cmdList("e cmd.pdc=?").contains(QStringLiteral("r2dec"));
}

QString CutterCore::getDecompiledCodeR2Dec(RVA addr)
{
    return cmd("pdd @ " + QString::number(addr));
}


QJsonDocument CutterCore::getFileInfo()
{
    return cmdj("ij");
}

QJsonDocument CutterCore::getFileVersionInfo()
{
    return cmdj("iVj");
}

QJsonDocument CutterCore::getSignatureInfo()
{
    return cmdj("iCj");
}

QJsonDocument CutterCore::getStack(int size)
{
    return cmdj("pxrj " + QString::number(size) + " @ r:SP");
}

QJsonDocument CutterCore::getRegisterValues()
{
    return cmdj("drj");
}

QList<RegisterRefDescription> CutterCore::getRegisterRefs()
{
    QList<RegisterRefDescription> ret;
    QJsonArray registerRefArray = cmdj("drrj").array();

    for (const QJsonValue &value : registerRefArray) {
        QJsonObject regRefObject = value.toObject();

        RegisterRefDescription regRef;

        regRef.reg = regRefObject[RJsonKey::reg].toString();
        regRef.value = regRefObject[RJsonKey::value].toString();
        regRef.ref = regRefObject[RJsonKey::ref].toString();

        ret << regRef;
    }

    return ret;
}

QList<VariableDescription> CutterCore::getVariables(RVA at)
{
    QList<VariableDescription> ret;
    QJsonObject varsObject = cmdj(QString("afvj @ %1").arg(at)).object();

    auto addVars = [&](VariableDescription::RefType refType, const QJsonArray &array) {
        for (const QJsonValue &varValue : array) {
            QJsonObject varObject = varValue.toObject();
            VariableDescription desc;
            desc.refType = refType;
            desc.name = varObject["name"].toString();
            desc.type = varObject["type"].toString();
            ret << desc;
        }
    };

    addVars(VariableDescription::RefType::SP, varsObject["sp"].toArray());
    addVars(VariableDescription::RefType::BP, varsObject["bp"].toArray());
    addVars(VariableDescription::RefType::Reg, varsObject["reg"].toArray());

    return ret;
}

QJsonObject CutterCore::getRegisterJson()
{
    QJsonArray registerRefArray = cmdj("drrj").array();
    QJsonObject registerJson;

    for (const QJsonValue &value : registerRefArray) {
        QJsonObject regRefObject = value.toObject();

        QJsonObject registers;

        registers.insert(RJsonKey::value, regRefObject[RJsonKey::value]);
        registers.insert(RJsonKey::ref, regRefObject[RJsonKey::ref]);

        registerJson.insert(regRefObject[RJsonKey::reg].toString(), registers);
    }
    return registerJson;
}

QString CutterCore::getRegisterName(QString registerRole)
{
    return cmd("drn " + registerRole).trimmed();
}

RVA CutterCore::getProgramCounterValue()
{
    bool ok;
    if (currentlyDebugging) {
        RVA addr = cmd("dr?`drn PC`").toULongLong(&ok, 16);
        if (ok) {
            return addr;
        }
    }
    return RVA_INVALID;
}

void CutterCore::setRegister(QString regName, QString regValue)
{
    cmd("dr " + regName + "=" + regValue);
    emit registersChanged();
    emit refreshCodeViews();
}

void CutterCore::startDebug()
{
    if (!currentlyDebugging) {
        offsetPriorDebugging = getOffset();
    }
    cmd("ood");
    emit registersChanged();
    if (!currentlyDebugging) {
        setConfig("asm.flags", false);
        currentlyDebugging = true;
        emit changeDebugView();
        emit flagsChanged();
        emit refreshCodeViews();
    }
    emit stackChanged();
}

void CutterCore::startEmulation()
{
    if (!currentlyDebugging) {
        offsetPriorDebugging = getOffset();
    }
    // clear registers, init esil state, stack, progcounter at current seek
    cmd("ar0; aei; aeim; aeip");
    emit registersChanged();
    if (!currentlyDebugging || !currentlyEmulating) {
        // prevent register flags from appearing during debug/emul
        setConfig("asm.flags", false);
        // allows to view self-modifying code changes or other binary changes
        setConfig("io.cache", true);
        currentlyDebugging = true;
        currentlyEmulating = true;
        emit changeDebugView();
        emit flagsChanged();
    }
    emit stackChanged();
    emit refreshCodeViews();
}

void CutterCore::attachDebug(int pid)
{
    if (!currentlyDebugging) {
        offsetPriorDebugging = getOffset();
    }
    // attach to process with dbg plugin
    cmd("o-*; e cfg.debug = true; o+ dbg://" + QString::number(pid));
    QString programCounterValue = cmd("dr?`drn PC`").trimmed();
    seek(programCounterValue);
    emit registersChanged();
    if (!currentlyDebugging || !currentlyEmulating) {
        // prevent register flags from appearing during debug/emul
        setConfig("asm.flags", false);
        currentlyDebugging = true;
        currentlyOpenFile = getConfig("file.path");
        currentlyAttachedToPID = pid;
        emit flagsChanged();
        emit changeDebugView();
    }
}

void CutterCore::stopDebug()
{
    if (currentlyDebugging) {
        if (currentlyEmulating) {
            cmd("aeim-; aei-; wcr; .ar-");
            currentlyEmulating = false;
        } else if (currentlyAttachedToPID != -1) {
            cmd(QString("dp- %1; o %2; .ar-").arg(QString::number(currentlyAttachedToPID), currentlyOpenFile));
            currentlyAttachedToPID = -1;
        } else {
            cmd("dk 9; oo; .ar-");
        }
        seek(offsetPriorDebugging);
        setConfig("asm.flags", true);
        setConfig("io.cache", false);
        currentlyDebugging = false;
        emit flagsChanged();
        emit changeDefinedView();
    }
}

void CutterCore::continueDebug()
{
    if (currentlyDebugging) {
        if (currentlyEmulating) {
            cmdEsil("aec");
        } else {
            cmd("dc");
        }
        emit registersChanged();
        emit refreshCodeViews();
    }
}

void CutterCore::continueUntilDebug(QString offset)
{
    if (currentlyDebugging) {
        if (currentlyEmulating) {
            cmdEsil("aecu " + offset);
        } else {
            cmd("dcu " + offset);
        }
        emit registersChanged();
        emit refreshCodeViews();
    }
}

void CutterCore::continueUntilCall()
{
    if (currentlyDebugging) {
        if (currentlyEmulating) {
            cmdEsil("aecc");
        } else {
            cmd("dcc");
        }
        QString programCounterValue = cmd("dr?`drn PC`").trimmed();
        seek(programCounterValue);
        emit registersChanged();
    }
}

void CutterCore::continueUntilSyscall()
{
    if (currentlyDebugging) {
        if (currentlyEmulating) {
            cmdEsil("aecs");
        } else {
            cmd("dcs");
        }
        QString programCounterValue = cmd("dr?`drn PC`").trimmed();
        seek(programCounterValue);
        emit registersChanged();
    }
}

void CutterCore::stepDebug()
{
    if (currentlyDebugging) {
        cmdEsil("ds");
        QString programCounterValue = cmd("dr?`drn PC`").trimmed();
        seek(programCounterValue);
        emit registersChanged();
    }
}

void CutterCore::stepOverDebug()
{
    if (currentlyDebugging) {
        cmdEsil("dso");
        QString programCounterValue = cmd("dr?`drn PC`").trimmed();
        seek(programCounterValue);
        emit registersChanged();
    }
}

void CutterCore::stepOutDebug()
{
    if (currentlyDebugging) {
        cmd("dsf");
        QString programCounterValue = cmd("dr?`drn PC`").trimmed();
        seek(programCounterValue);
        emit registersChanged();
    }
}

QStringList CutterCore::getDebugPlugins()
{
    QStringList plugins;
    QJsonArray pluginArray = cmdj("dLj").array();

    for (const QJsonValue &value : pluginArray) {
        QJsonObject pluginObject = value.toObject();

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

void CutterCore::toggleBreakpoint(RVA addr)
{
    cmd("dbs " + RAddressString(addr));
    emit instructionChanged(addr);
    emit breakpointsChanged();
}

void CutterCore::toggleBreakpoint(QString addr)
{
    cmd("dbs " + addr);
    emit instructionChanged(addr.toULongLong());
    emit breakpointsChanged();
}

void CutterCore::delBreakpoint(RVA addr)
{
    cmd("db- " + RAddressString(addr));
    emit instructionChanged(addr);
    emit breakpointsChanged();
}

void CutterCore::delAllBreakpoints()
{
    cmd("db-*");
    emit refreshCodeViews();
}

void CutterCore::enableBreakpoint(RVA addr)
{
    cmd("dbe " + RAddressString(addr));
    emit instructionChanged(addr);
    emit breakpointsChanged();
}

void CutterCore::disableBreakpoint(RVA addr)
{
    cmd("dbd " + RAddressString(addr));
    emit instructionChanged(addr);
    emit breakpointsChanged();
}

QList<BreakpointDescription> CutterCore::getBreakpoints()
{
    QList<BreakpointDescription> ret;
    QJsonArray breakpointArray = cmdj("dbj").array();

    for (const QJsonValue &value : breakpointArray) {
        QJsonObject bpObject = value.toObject();

        BreakpointDescription bp;

        bp.addr = bpObject[RJsonKey::addr].toVariant().toULongLong();
        bp.size = bpObject[RJsonKey::size].toInt();
        bp.permission = bpObject[RJsonKey::prot].toString();
        bp.hw = bpObject[RJsonKey::hw].toBool();
        bp.trace = bpObject[RJsonKey::trace].toBool();
        bp.enabled = bpObject[RJsonKey::enabled].toBool();

        ret << bp;
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

bool CutterCore::isBreakpoint(const QList<RVA> &breakpoints, RVA addr)
{
    return breakpoints.contains(addr);
}

QJsonDocument CutterCore::getBacktrace()
{
    return cmdj("dbtj");
}

QList<ProcessDescription> CutterCore::getAllProcesses()
{
    QList<ProcessDescription> ret;
    QJsonArray processArray = cmdj("dplj").array();

    for (const QJsonValue &value : processArray) {
        QJsonObject procObject = value.toObject();

        ProcessDescription proc;

        proc.pid = procObject[RJsonKey::pid].toInt();
        proc.uid = procObject[RJsonKey::uid].toInt();
        proc.status = procObject[RJsonKey::status].toString();
        proc.path = procObject[RJsonKey::path].toString();

        ret << proc;
    }

    return ret;
}

QList<MemoryMapDescription> CutterCore::getMemoryMap()
{
    QList<MemoryMapDescription> ret;
    QJsonArray memoryMapArray = cmdj("dmj").array();

    for (const QJsonValue &value : memoryMapArray) {
        QJsonObject memMapObject = value.toObject();

        MemoryMapDescription memMap;

        memMap.name = memMapObject[RJsonKey::name].toString();
        memMap.fileName = memMapObject[RJsonKey::file].toString();
        memMap.addrStart = memMapObject[RJsonKey::addr].toVariant().toULongLong();
        memMap.addrEnd = memMapObject[RJsonKey::addr_end].toVariant().toULongLong();
        memMap.type = memMapObject[RJsonKey::type].toString();
        memMap.permission = memMapObject[RJsonKey::perm].toString();

        ret << memMap;
    }

    return ret;
}

QStringList CutterCore::getStats()
{
    QStringList stats;
    cmd("fs functions");
    stats << cmd("f~?").trimmed();

    QString imps = cmd("ii~?").trimmed();
    stats << imps;

    cmd("fs symbols");
    stats << cmd("f~?").trimmed();
    cmd("fs strings");
    stats << cmd("f~?").trimmed();
    cmd("fs relocs");
    stats << cmd("f~?").trimmed();
    cmd("fs sections");
    stats << cmd("f~?").trimmed();
    cmd("fs *");
    stats << cmd("f~?").trimmed();

    return stats;
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

    setConfig("anal.hasnext", false);
    setConfig("asm.lines.call", false);

    setConfig("cfg.fortunes.tts", false);

    // Colors
    setConfig("scr.color", COLOR_MODE_DISABLED);

    // Don't show hits
    setConfig("search.flags", false);
}

QList<RVA> CutterCore::getSeekHistory()
{
    CORE_LOCK();
    QList<RVA> ret;

    QJsonArray jsonArray = cmdj("sj").array();
    for (const QJsonValue &value : jsonArray)
        ret << value.toVariant().toULongLong();

    return ret;
}

QStringList CutterCore::getAsmPluginNames()
{
    CORE_LOCK();
    RListIter *it;
    QStringList ret;

    RAsmPlugin *ap;
    CutterRListForeach(core_->assembler->plugins, it, RAsmPlugin, ap) {
        ret << ap->name;
    }

    return ret;
}

QStringList CutterCore::getAnalPluginNames()
{
    CORE_LOCK();
    RListIter *it;
    QStringList ret;

    RAnalPlugin *ap;
    CutterRListForeach(core_->anal->plugins, it, RAnalPlugin, ap) {
        ret << ap->name;
    }

    return ret;
}

QStringList CutterCore::getProjectNames()
{
    CORE_LOCK();
    QStringList ret;

    QJsonArray jsonArray = cmdj("Pj").array();
    for (const QJsonValue &value : jsonArray)
        ret.append(value.toString());

    return ret;
}

QList<RBinPluginDescription> CutterCore::getRBinPluginDescriptions(const QString &type)
{
    QList<RBinPluginDescription> ret;

    QJsonObject jsonRoot = cmdj("iLj").object();
    for (const QString &key : jsonRoot.keys()) {
        if (!type.isNull() && key != type)
            continue;

        QJsonArray pluginArray = jsonRoot[key].toArray();

        for (const QJsonValue &pluginValue : pluginArray) {
            QJsonObject pluginObject = pluginValue.toObject();

            RBinPluginDescription desc;

            desc.name = pluginObject[RJsonKey::name].toString();
            desc.description = pluginObject[RJsonKey::description].toString();
            desc.license = pluginObject[RJsonKey::license].toString();
            desc.type = key;

            ret.append(desc);
        }
    }

    return ret;
}

QList<RIOPluginDescription> CutterCore::getRIOPluginDescriptions()
{
    QList<RIOPluginDescription> ret;

    QJsonArray plugins = cmdj("oLj").object()["io_plugins"].toArray();
    for (const QJsonValue &pluginValue : plugins) {
        QJsonObject pluginObject = pluginValue.toObject();

        RIOPluginDescription plugin;

        plugin.name = pluginObject["name"].toString();
        plugin.description = pluginObject["description"].toString();
        plugin.license = pluginObject["license"].toString();
        plugin.permissions = pluginObject["permissions"].toString();
        for (const auto &uri : pluginObject["uris"].toArray()) {
            plugin.uris << uri.toString();
        }

        ret << plugin;
    }

    return ret;
}

QList<RCorePluginDescription> CutterCore::getRCorePluginDescriptions()
{
    QList<RCorePluginDescription> ret;

    QJsonArray plugins = cmdj("Lsj").array();
    for (const QJsonValue &pluginValue : plugins) {
        QJsonObject pluginObject = pluginValue.toObject();

        RCorePluginDescription plugin;

        plugin.name = pluginObject["Name"].toString();
        plugin.description = pluginObject["Description"].toString();

        ret << plugin;
    }

    return ret;
}

QList<RAsmPluginDescription> CutterCore::getRAsmPluginDescriptions()
{
    CORE_LOCK();
    RListIter *it;
    QList<RAsmPluginDescription> ret;

    RAsmPlugin *ap;
    CutterRListForeach(core_->assembler->plugins, it, RAsmPlugin, ap) {
        RAsmPluginDescription plugin;

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
    funcList.reserve(r_list_length(core_->anal->fcns));

    RListIter *iter;
    RAnalFunction *fcn;
    CutterRListForeach (core_->anal->fcns, iter, RAnalFunction, fcn) {
        FunctionDescription function;
        function.offset = fcn->addr;
        function.size = r_anal_fcn_size(fcn);
        function.nargs = r_anal_var_count(core_->anal, fcn, 'b', 1) +
            r_anal_var_count(core_->anal, fcn, 'r', 1) +
            r_anal_var_count(core_->anal, fcn, 's', 1);
        function.nlocals = r_anal_var_count(core_->anal, fcn, 'b', 0) +
            r_anal_var_count(core_->anal, fcn, 'r', 0) +
            r_anal_var_count(core_->anal, fcn, 's', 0);
        function.nbbs = r_list_length (fcn->bbs);
        function.calltype = fcn->cc ? QString::fromUtf8(fcn->cc) : QString();
        function.name = fcn->name ? QString::fromUtf8(fcn->name) : QString();
        function.edges = r_anal_fcn_count_edges(fcn, nullptr);
        function.stackframe = fcn->maxstack;
        funcList.append(function);
    }

    return funcList;
}

QList<ImportDescription> CutterCore::getAllImports()
{
    CORE_LOCK();
    QList<ImportDescription> ret;

    QJsonArray importsArray = cmdj("iij").array();

    for (const QJsonValue &value : importsArray) {
        QJsonObject importObject = value.toObject();

        ImportDescription import;

        import.plt = importObject[RJsonKey::plt].toVariant().toULongLong();
        import.ordinal = importObject[RJsonKey::ordinal].toInt();
        import.bind = importObject[RJsonKey::bind].toString();
        import.type = importObject[RJsonKey::type].toString();
        import.name = importObject[RJsonKey::name].toString();

        ret << import;
    }

    return ret;
}

QList<ExportDescription> CutterCore::getAllExports()
{
    CORE_LOCK();
    QList<ExportDescription> ret;

    QJsonArray exportsArray = cmdj("iEj").array();

    for (const QJsonValue &value : exportsArray) {
        QJsonObject exportObject = value.toObject();

        ExportDescription exp;

        exp.vaddr = exportObject[RJsonKey::vaddr].toVariant().toULongLong();
        exp.paddr = exportObject[RJsonKey::paddr].toVariant().toULongLong();
        exp.size = exportObject[RJsonKey::size].toVariant().toULongLong();
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
    RListIter *it;

    QList<SymbolDescription> ret;

    RBinSymbol *bs;
    if (core_ && core_->bin && core_->bin->cur && core_->bin->cur->o) {
        CutterRListForeach(core_->bin->cur->o->symbols, it, RBinSymbol, bs) {
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
        RBinAddr *entry;
        CutterRListForeach(core_->bin->cur->o->entries, it, RBinAddr, entry) {
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

    QJsonArray headersArray = cmdj("ihj").array();

    for (const QJsonValue &value : headersArray) {
        QJsonObject headerObject = value.toObject();

        HeaderDescription header;

        header.vaddr = headerObject[RJsonKey::vaddr].toVariant().toULongLong();
        header.paddr = headerObject[RJsonKey::paddr].toVariant().toULongLong();
        header.value = headerObject[RJsonKey::comment].toString();
        header.name = headerObject[RJsonKey::name].toString();

        ret << header;
    }

    return ret;
}

QList<ZignatureDescription> CutterCore::getAllZignatures()
{
    CORE_LOCK();
    QList<ZignatureDescription> zignatures;

    QJsonArray zignaturesArray = cmdj("zj").array();

    for (const QJsonValue &value : zignaturesArray) {
        QJsonObject zignatureObject = value.toObject();

        ZignatureDescription zignature;

        zignature.name = zignatureObject[RJsonKey::name].toString();
        zignature.bytes = zignatureObject[RJsonKey::bytes].toString();
        zignature.offset = zignatureObject[RJsonKey::offset].toVariant().toULongLong();
        for (const QJsonValue &ref : zignatureObject[RJsonKey::refs].toArray()) {
            zignature.refs << ref.toString();
        }

        QJsonObject graphObject = zignatureObject[RJsonKey::graph].toObject();
        zignature.cc = graphObject[RJsonKey::cc].toVariant().toULongLong();
        zignature.nbbs = graphObject[RJsonKey::nbbs].toVariant().toULongLong();
        zignature.edges = graphObject[RJsonKey::edges].toVariant().toULongLong();
        zignature.ebbs = graphObject[RJsonKey::ebbs].toVariant().toULongLong();

        zignatures << zignature;
    }

    return zignatures;
}

QList<CommentDescription> CutterCore::getAllComments(const QString &filterType)
{
    CORE_LOCK();
    QList<CommentDescription> ret;

    QJsonArray commentsArray = cmdj("CCj").array();
    for (const QJsonValue &value : commentsArray) {
        QJsonObject commentObject = value.toObject();

        QString type = commentObject[RJsonKey::type].toString();
        if (type != filterType)
            continue;

        CommentDescription comment;
        comment.offset = commentObject[RJsonKey::offset].toVariant().toULongLong();
        comment.name = commentObject[RJsonKey::name].toString();

        ret << comment;
    }
    return ret;
}

QList<RelocDescription> CutterCore::getAllRelocs()
{
    CORE_LOCK();
    QList<RelocDescription> ret;

    if (core_ && core_->bin && core_->bin->cur && core_->bin->cur->o) {
        auto relocs = core_->bin->cur->o->relocs;
        RBIter iter;
        RBinReloc *br;
        r_rbtree_foreach (relocs, iter, br, RBinReloc, vrb) {
            RelocDescription reloc;

            reloc.vaddr = br->vaddr;
            reloc.paddr = br->paddr;
            reloc.type = (br->additive ? "ADD_" : "SET_") + QString::number(br->type);

            if (br->import)
                reloc.name = br->import->name;
            else
                reloc.name = QString("reloc_%1").arg(QString::number(br->vaddr, 16));

            ret << reloc;
        }
    }

    return ret;
}

QList<StringDescription> CutterCore::getAllStrings()
{
    return parseStringsJson(cmdjTask("izzj"));
}

QList<StringDescription> CutterCore::parseStringsJson(const QJsonDocument &doc)
{
    QList<StringDescription> ret;

    QJsonArray stringsArray = doc.array();
    for (const QJsonValue &value : stringsArray) {
        QJsonObject stringObject = value.toObject();

        StringDescription string;

        string.string = QString(QByteArray::fromBase64(stringObject[RJsonKey::string].toVariant().toByteArray()));
        string.vaddr = stringObject[RJsonKey::vaddr].toVariant().toULongLong();
        string.type = stringObject[RJsonKey::type].toString();
        string.size = stringObject[RJsonKey::size].toVariant().toUInt();
        string.length = stringObject[RJsonKey::length].toVariant().toUInt();
        string.section = stringObject[RJsonKey::section].toString();

        ret << string;
    }

    return ret;
}

QList<FlagspaceDescription> CutterCore::getAllFlagspaces()
{
    CORE_LOCK();
    QList<FlagspaceDescription> ret;

    QJsonArray flagspacesArray = cmdj("fsj").array();
    for (const QJsonValue &value : flagspacesArray) {
        QJsonObject flagspaceObject = value.toObject();

        FlagspaceDescription flagspace;

        flagspace.name = flagspaceObject[RJsonKey::name].toString();

        ret << flagspace;
    }
    return ret;
}

QList<FlagDescription> CutterCore::getAllFlags(QString flagspace)
{
    CORE_LOCK();
    QList<FlagDescription> ret;

    if (!flagspace.isEmpty())
        cmd("fs " + flagspace);
    else
        cmd("fs *");

    QJsonArray flagsArray = cmdj("fj").array();
    for (const QJsonValue &value : flagsArray) {
        QJsonObject flagObject = value.toObject();

        FlagDescription flag;

        flag.offset = flagObject[RJsonKey::offset].toVariant().toULongLong();
        flag.size = flagObject[RJsonKey::size].toVariant().toULongLong();
        flag.name = flagObject[RJsonKey::name].toString();

        ret << flag;
    }
    return ret;
}

QList<SectionDescription> CutterCore::getAllSections()
{
    CORE_LOCK();
    QList<SectionDescription> sections;

    QJsonDocument sectionsDoc = cmdj("iSj entropy");
    QJsonObject sectionsObj = sectionsDoc.object();
    QJsonArray sectionsArray = sectionsObj[RJsonKey::sections].toArray();

    for (const QJsonValue &value : sectionsArray) {
        QJsonObject sectionObject = value.toObject();

        QString name = sectionObject[RJsonKey::name].toString();
        if (name.isEmpty())
            continue;

        SectionDescription section;
        section.name = name;
        section.vaddr = sectionObject[RJsonKey::vaddr].toVariant().toULongLong();
        section.vsize = sectionObject[RJsonKey::vsize].toVariant().toULongLong();
        section.paddr = sectionObject[RJsonKey::paddr].toVariant().toULongLong();
        section.size = sectionObject[RJsonKey::size].toVariant().toULongLong();
        section.perm = sectionObject[RJsonKey::perm].toString();
        section.entropy =  sectionObject[RJsonKey::entropy].toString();

        sections << section;
    }
    return sections;
}

QStringList CutterCore::getSectionList()
{
    CORE_LOCK();
    QStringList ret;

    QJsonArray sectionsArray = cmdj("iSj").array();
    for (const QJsonValue &value : sectionsArray) {
        ret << value.toObject()[RJsonKey::name].toString();
    }
    return ret;
}

QList<SegmentDescription> CutterCore::getAllSegments()
{
    CORE_LOCK();
    QList<SegmentDescription> ret;

    QJsonArray segments = cmdj("iSSj").array();

    for (const QJsonValue &value : segments) {
        QJsonObject segmentObject = value.toObject();

        QString name = segmentObject[RJsonKey::name].toString();
        if (name.isEmpty())
            continue;

        SegmentDescription segment;
        segment.name = name;
        segment.vaddr = segmentObject[RJsonKey::vaddr].toVariant().toULongLong();
        segment.paddr = segmentObject[RJsonKey::paddr].toVariant().toULongLong();
        segment.size = segmentObject[RJsonKey::size].toVariant().toULongLong();
        segment.vsize = segmentObject[RJsonKey::vsize].toVariant().toULongLong();
        segment.perm =  segmentObject[RJsonKey::perm].toString();

        ret << segment;
    }
    return ret;
}

QList<EntrypointDescription> CutterCore::getAllEntrypoint()
{
    CORE_LOCK();
    QList<EntrypointDescription> ret;

    QJsonArray entrypointsArray = cmdj("iej").array();
    for (const QJsonValue &value : entrypointsArray) {
        QJsonObject entrypointObject = value.toObject();

        EntrypointDescription entrypoint;

        entrypoint.vaddr = entrypointObject[RJsonKey::vaddr].toVariant().toULongLong();
        entrypoint.paddr = entrypointObject[RJsonKey::paddr].toVariant().toULongLong();
        entrypoint.baddr = entrypointObject[RJsonKey::baddr].toVariant().toULongLong();
        entrypoint.laddr = entrypointObject[RJsonKey::laddr].toVariant().toULongLong();
        entrypoint.haddr = entrypointObject[RJsonKey::haddr].toVariant().toULongLong();
        entrypoint.type = entrypointObject[RJsonKey::type].toString();

        ret << entrypoint;
    }
    return ret;
}

QList<BinClassDescription> CutterCore::getAllClassesFromBin()
{
    CORE_LOCK();
    QList<BinClassDescription> ret;

    QJsonArray classesArray = cmdj("icj").array();
    for (const QJsonValue &value : classesArray) {
        QJsonObject classObject = value.toObject();

        BinClassDescription cls;

        cls.name = classObject[RJsonKey::classname].toString();
        cls.addr = classObject[RJsonKey::addr].toVariant().toULongLong();
        cls.index = classObject[RJsonKey::index].toVariant().toULongLong();

        for (const QJsonValue &value2 : classObject[RJsonKey::methods].toArray()) {
            QJsonObject methObject = value2.toObject();

            BinClassMethodDescription meth;

            meth.name = methObject[RJsonKey::name].toString();
            meth.addr = methObject[RJsonKey::addr].toVariant().toULongLong();

            cls.methods << meth;
        }

        for (const QJsonValue &value2 : classObject[RJsonKey::fields].toArray()) {
            QJsonObject fieldObject = value2.toObject();

            BinClassFieldDescription field;

            field.name = fieldObject[RJsonKey::name].toString();
            field.addr = fieldObject[RJsonKey::addr].toVariant().toULongLong();

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

    QJsonArray flagsArray = cmdj("fj@F:classes").array();
    for (const QJsonValue &value : flagsArray) {
        QJsonObject flagObject = value.toObject();

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
            desc->addr = flagObject[RJsonKey::offset].toVariant().toULongLong();
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
            meth.addr = flagObject[RJsonKey::offset].toVariant().toULongLong();
            classDesc->methods << meth;
            continue;
        }
    }
    return ret;
}

QList<QString> CutterCore::getAllAnalClasses(bool sorted)
{
    QList<QString> ret;

    SdbList *l = r_anal_class_get_all(core_->anal, sorted);
    if (!l) {
        return ret;
    }
    ret.reserve(static_cast<int>(l->length));

    SdbListIter *it;
    void *entry;
    ls_foreach(l, it, entry) {
        auto kv = reinterpret_cast<SdbKv *>(entry);
        ret.append(QString::fromUtf8(reinterpret_cast<const char *>(kv->base.key)));
    }
    ls_free(l);

    return ret;
}

QList<AnalMethodDescription> CutterCore::getAnalClassMethods(const QString &cls)
{
    QList<AnalMethodDescription> ret;

    RVector *meths = r_anal_class_method_get_all(core_->anal, cls.toUtf8().constData());
    if (!meths) {
        return ret;
    }

    ret.reserve(static_cast<int>(meths->len));
    RAnalMethod *meth;
    CutterRVectorForeach(meths, meth, RAnalMethod) {
        AnalMethodDescription desc;
        desc.name = QString::fromUtf8(meth->name);
        desc.addr = meth->addr;
        desc.vtableOffset = meth->vtable_offset;
        ret.append(desc);
    }
    r_vector_free(meths);

    return ret;
}

QList<AnalBaseClassDescription> CutterCore::getAnalClassBaseClasses(const QString &cls)
{
    QList<AnalBaseClassDescription> ret;

    RVector *bases = r_anal_class_base_get_all(core_->anal, cls.toUtf8().constData());
    if (!bases) {
        return ret;
    }

    ret.reserve(static_cast<int>(bases->len));
    RAnalBaseClass *base;
    CutterRVectorForeach(bases, base, RAnalBaseClass) {
        AnalBaseClassDescription desc;
        desc.id = QString::fromUtf8(base->id);
        desc.offset = base->offset;
        desc.className = QString::fromUtf8(base->class_name);
        ret.append(desc);
    }
    r_vector_free(bases);

    return ret;
}

QList<AnalVTableDescription> CutterCore::getAnalClassVTables(const QString &cls)
{
    QList<AnalVTableDescription> acVtables;

    RVector *vtables = r_anal_class_vtable_get_all(core_->anal, cls.toUtf8().constData());
    if (!vtables) {
        return acVtables;
    }

    acVtables.reserve(static_cast<int>(vtables->len));
    RAnalVTable *vtable;
    CutterRVectorForeach(vtables, vtable, RAnalVTable) {
        AnalVTableDescription desc;
        desc.id = QString::fromUtf8(vtable->id);
        desc.offset = vtable->offset;
        desc.addr = vtable->addr;
        acVtables.append(desc);
    }
    r_vector_free(vtables);

    return acVtables;
}

void CutterCore::createNewClass(const QString &cls)
{
    r_anal_class_create(core_->anal, cls.toUtf8().constData());
}

void CutterCore::renameClass(const QString &oldName, const QString &newName)
{
    r_anal_class_rename(core_->anal, oldName.toUtf8().constData(), newName.toUtf8().constData());
}

void CutterCore::deleteClass(const QString &cls)
{
    r_anal_class_delete(core_->anal, cls.toUtf8().constData());
}

bool CutterCore::getAnalMethod(const QString &cls, const QString &meth, AnalMethodDescription *desc)
{
    RAnalMethod analMeth;
    if (r_anal_class_method_get(core_->anal, cls.toUtf8().constData(), meth.toUtf8().constData(), &analMeth) != R_ANAL_CLASS_ERR_SUCCESS) {
        return false;
    }
    desc->name = QString::fromUtf8(analMeth.name);
    desc->addr = analMeth.addr;
    desc->vtableOffset = analMeth.vtable_offset;
    r_anal_class_method_fini(&analMeth);
    return true;
}

void CutterCore::setAnalMethod(const QString &className, const AnalMethodDescription &meth)
{
    RAnalMethod analMeth;
    analMeth.name = strdup (meth.name.toUtf8().constData());
    analMeth.addr = meth.addr;
    analMeth.vtable_offset = meth.vtableOffset;
    r_anal_class_method_set(core_->anal, className.toUtf8().constData(), &analMeth);
    r_anal_class_method_fini(&analMeth);
}

void CutterCore::renameAnalMethod(const QString &className, const QString &oldMethodName, const QString &newMethodName)
{
    r_anal_class_method_rename(core_->anal, className.toUtf8().constData(), oldMethodName.toUtf8().constData(), newMethodName.toUtf8().constData());
}

QList<ResourcesDescription> CutterCore::getAllResources()
{
    CORE_LOCK();
    QList<ResourcesDescription> resources;

    QJsonArray resourcesArray = cmdj("iRj").array();
    for (const QJsonValue &value : resourcesArray) {
        QJsonObject resourceObject = value.toObject();

        ResourcesDescription res;

        res.name = resourceObject[RJsonKey::name].toInt();
        res.vaddr = resourceObject[RJsonKey::vaddr].toVariant().toULongLong();
        res.index = resourceObject[RJsonKey::index].toVariant().toULongLong();
        res.type = resourceObject[RJsonKey::type].toString();
        res.size = resourceObject[RJsonKey::size].toVariant().toULongLong();
        res.lang = resourceObject[RJsonKey::lang].toString();

        resources << res;
    }
    return resources;
}

QList<VTableDescription> CutterCore::getAllVTables()
{
    CORE_LOCK();
    QList<VTableDescription> vtables;

    QJsonArray vTablesArray = cmdj("avj").array();
    for (const QJsonValue &vTableValue : vTablesArray) {
        QJsonObject vTableObject = vTableValue.toObject();

        VTableDescription res;

        res.addr = vTableObject[RJsonKey::offset].toVariant().toULongLong();
        QJsonArray methodArray = vTableObject[RJsonKey::methods].toArray();

        for (const QJsonValue &methodValue : methodArray) {
            QJsonObject methodObject = methodValue.toObject();

            BinClassMethodDescription method;

            method.addr = methodObject[RJsonKey::offset].toVariant().toULongLong();
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

QList<TypeDescription> CutterCore::getAllPrimitiveTypes()
{
    CORE_LOCK();
    QList<TypeDescription> primitiveTypes;

    QJsonArray typesArray = cmdj("tj").array();
    for (const QJsonValue &value : typesArray) {
        QJsonObject typeObject = value.toObject();

        TypeDescription exp;

        exp.type = typeObject[RJsonKey::type].toString();
        exp.size = typeObject[RJsonKey::size].toVariant().toULongLong();
        exp.format = typeObject[RJsonKey::format].toString();
        exp.category = tr("Primitive");
        primitiveTypes << exp;
    }

    return primitiveTypes;
}

QList<TypeDescription> CutterCore::getAllUnions()
{
    CORE_LOCK();
    QList<TypeDescription> unions;

    QJsonArray typesArray = cmdj("tuj").array();
    for (const QJsonValue value: typesArray) {
        QJsonObject typeObject = value.toObject();

        TypeDescription exp;

        exp.type = typeObject[RJsonKey::type].toString();
        exp.size = typeObject[RJsonKey::size].toVariant().toULongLong();
        exp.category = "Union";
        unions << exp;
    }

    return unions;
}

QList<TypeDescription> CutterCore::getAllStructs()
{
    CORE_LOCK();
    QList<TypeDescription> structs;

    QJsonArray typesArray = cmdj("tsj").array();
    for (const QJsonValue value: typesArray) {
        QJsonObject typeObject = value.toObject();

        TypeDescription exp;

        exp.type = typeObject[RJsonKey::type].toString();
        exp.size = typeObject[RJsonKey::size].toVariant().toULongLong();
        exp.category = "Struct";
        structs << exp;
    }

    return structs;
}

QList<TypeDescription> CutterCore::getAllEnums()
{
    CORE_LOCK();
    QList<TypeDescription> enums;

    QJsonObject typesObject = cmdj("tej").object();
    for (QString key: typesObject.keys()) {
        TypeDescription exp;
        exp.type = key;
        exp.size = 0;
        exp.category = "Enum";
        enums << exp;
    }

    return enums;
}

QList<TypeDescription> CutterCore::getAllTypedefs()
{
    CORE_LOCK();
    QList<TypeDescription> typeDefs;

    QJsonObject typesObject = cmdj("ttj").object();
    for (QString key: typesObject.keys()) {
        TypeDescription exp;
        exp.type = key;
        exp.size = 0;
        exp.category = "Typedef";
        typeDefs << exp;
    }

    return typeDefs;
}

QString CutterCore::addTypes(const char *str)
{
    char *error_msg = nullptr;
    char *parsed = r_parse_c_string(core_->anal, str, &error_msg);
    QString error;

    if (!parsed) {
         if (error_msg) {
             error = error_msg;
             r_mem_free(error_msg);
         }
         return error;
    }

    r_anal_save_parsed_type(core_->anal, parsed);
    r_mem_free(parsed);

    if (error_msg) {
        error = error_msg;
        r_mem_free(error_msg);
    }

    return error;
}

bool CutterCore::isAddressMapped(RVA addr)
{
    // If value returned by "om. @ addr" is empty means that address is not mapped
    return !Core()->cmd(QString("om. @ %1").arg(addr)).isEmpty();
}

QList<SearchDescription> CutterCore::getAllSearch(QString search_for, QString space)
{
    CORE_LOCK();
    QList<SearchDescription> searchRef;

    QJsonArray searchArray = cmdj(space + QString(" ") + search_for).array();

    if (space == "/Rj") {
        for (const QJsonValue &value : searchArray) {
            QJsonObject searchObject = value.toObject();

            SearchDescription exp;

            exp.code.clear();
            for (const QJsonValue &value2 : searchObject[RJsonKey::opcodes].toArray()) {
                QJsonObject gadget = value2.toObject();
                exp.code += gadget[RJsonKey::opcode].toString() + ";  ";
            }

            exp.offset =
                searchObject[RJsonKey::opcodes].toArray().first().toObject()[RJsonKey::offset].toVariant().toULongLong();
            exp.size = searchObject[RJsonKey::size].toVariant().toULongLong();

            searchRef << exp;
        }
    } else {
        for (const QJsonValue &value : searchArray) {
            QJsonObject searchObject = value.toObject();

            SearchDescription exp;

            exp.offset = searchObject[RJsonKey::offset].toVariant().toULongLong();
            exp.size = searchObject[RJsonKey::len].toVariant().toULongLong();
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

    QJsonObject statsObj = cmdj("p-j " + QString::number(blocksCount)).object();

    blockStats.from = statsObj[RJsonKey::from].toVariant().toULongLong();
    blockStats.to = statsObj[RJsonKey::to].toVariant().toULongLong();
    blockStats.blocksize = statsObj[RJsonKey::blocksize].toVariant().toULongLong();

    QJsonArray blocksArray = statsObj[RJsonKey::blocks].toArray();

    for (const QJsonValue &value : blocksArray) {
        QJsonObject blockObj = value.toObject();

        BlockDescription block;

        block.addr = blockObj[RJsonKey::offset].toVariant().toULongLong();
        block.size = blockObj[RJsonKey::size].toVariant().toULongLong();
        block.flags = blockObj[RJsonKey::flags].toInt(0);
        block.functions = blockObj[RJsonKey::functions].toInt(0);
        block.inFunctions = blockObj[RJsonKey::in_functions].toInt(0);
        block.comments = blockObj[RJsonKey::comments].toInt(0);
        block.symbols = blockObj[RJsonKey::symbols].toInt(0);
        block.strings = blockObj[RJsonKey::strings].toInt(0);

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

QList<XrefDescription> CutterCore::getXRefs(RVA addr, bool to, bool whole_function,
                                            const QString &filterType)
{
    QList<XrefDescription> xrefList = QList<XrefDescription>();

    QJsonArray xrefsArray;

    if (to) {
        xrefsArray = cmdj("axtj@" + QString::number(addr)).array();
    } else {
        xrefsArray = cmdj("axfj@" + QString::number(addr)).array();
    }

    for (const QJsonValue &value : xrefsArray) {
        QJsonObject xrefObject = value.toObject();

        XrefDescription xref;

        xref.type = xrefObject[RJsonKey::type].toString();

        if (!filterType.isNull() && filterType != xref.type)
            continue;

        xref.from = xrefObject[RJsonKey::from].toVariant().toULongLong();
        if (!to) {
            xref.from_str = RAddressString(xref.from);
        } else {
            QString fcn = xrefObject[RJsonKey::fcn_name].toString();
            if (!fcn.isEmpty()) {
                RVA fcnAddr = xrefObject[RJsonKey::fcn_addr].toVariant().toULongLong();
                xref.from_str = fcn + " + 0x" + QString::number(xref.from - fcnAddr, 16);
            } else {
                xref.from_str = RAddressString(xref.from);
            }
        }

        if (!whole_function && !to && xref.from != addr) {
            continue;
        }

        if (to && !xrefObject.contains(RJsonKey::to)) {
            xref.to = addr;
        } else {
            xref.to = xrefObject[RJsonKey::to].toVariant().toULongLong();
        }
        xref.to_str = Core()->cmd("fd " + QString::number(xref.to)).trimmed();

        xrefList << xref;
    }

    return xrefList;
}

void CutterCore::addFlag(RVA offset, QString name, RVA size)
{
    name = sanitizeStringForCommand(name);
    cmd(QString("f %1 %2 @ %3").arg(name).arg(size).arg(offset));
    emit flagsChanged();
}

void CutterCore::handleREvent(int type, void *data)
{
    switch (type) {
    case R_EVENT_CLASS_NEW: {
        auto ev = reinterpret_cast<REventClass *>(data);
        emit classNew(QString::fromUtf8(ev->name));
        break;
    }
    case R_EVENT_CLASS_DEL: {
        auto ev = reinterpret_cast<REventClass *>(data);
        emit classDeleted(QString::fromUtf8(ev->name));
        break;
    }
    case R_EVENT_CLASS_RENAME: {
        auto ev = reinterpret_cast<REventClassRename *>(data);
        emit classRenamed(QString::fromUtf8(ev->name_old), QString::fromUtf8(ev->name_new));
        break;
    }
    case R_EVENT_CLASS_ATTR_SET: {
        auto ev = reinterpret_cast<REventClassAttrSet *>(data);
        emit classAttrsChanged(QString::fromUtf8(ev->attr.class_name));
        break;
    }
    case R_EVENT_CLASS_ATTR_DEL: {
        auto ev = reinterpret_cast<REventClassAttr *>(data);
        emit classAttrsChanged(QString::fromUtf8(ev->class_name));
        break;
    }
    case R_EVENT_CLASS_ATTR_RENAME: {
        auto ev = reinterpret_cast<REventClassAttrRename *>(data);
        emit classAttrsChanged(QString::fromUtf8(ev->attr.class_name));
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

void CutterCore::triggerFunctionRenamed(const QString &prevName, const QString &newName)
{
    emit functionRenamed(prevName, newName);
}

void CutterCore::loadPDB(const QString &file)
{
    cmd("idp " + sanitizeStringForCommand(file));
}

void CutterCore::openProject(const QString &name)
{
    cmd("Po " + name);

    QString notes = QString::fromUtf8(QByteArray::fromBase64(cmd("Pnj").toUtf8()));
}

void CutterCore::saveProject(const QString &name)
{
    const QString &rv = cmd("Ps " + name.trimmed()).trimmed();
    const bool ok = rv == name.trimmed();
    cmd(QString("Pnj ") + notes.toUtf8().toBase64());
    emit projectSaved(ok, name);
}

void CutterCore::deleteProject(const QString &name)
{
    cmd("Pd " + name);
}

bool CutterCore::isProjectNameValid(const QString &name)
{
    // see is_valid_project_name() in libr/core/project.c
    static const QRegExp regexp(R"(^[a-zA-Z0-9\\\._:-]{1,}$)");
    return regexp.exactMatch(name) && !name.endsWith(".zip") ;
}

QList<DisassemblyLine> CutterCore::disassembleLines(RVA offset, int lines)
{
    QJsonArray array = cmdj(QString("pdJ ") + QString::number(lines) + QString(" @ ") + QString::number(
                                offset)).array();
    QList<DisassemblyLine> r;

    for (const QJsonValueRef &value : array) {
        QJsonObject object = value.toObject();
        DisassemblyLine line;
        line.offset = object[RJsonKey::offset].toVariant().toULongLong();
        line.text = ansiEscapeToHtml(object[RJsonKey::text].toString());
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

    return cmd(QString("%1 %2 @ %3")
                        .arg(command)
                        .arg(size)
                        .arg(address));
}

QByteArray CutterCore::hexStringToBytes(const QString &hex)
{
    QByteArray hexChars = hex.toUtf8();
    QByteArray bytes;
    bytes.reserve(hexChars.length() / 2);
    int size = r_hex_str2bin(hexChars.constData(), reinterpret_cast<ut8 *>(bytes.data()));
    bytes.resize(size);
    return bytes;
}

QString CutterCore::bytesToHexString(const QByteArray &bytes)
{
    QByteArray hex;
    hex.resize(bytes.length() * 2);
    r_hex_bin2str(reinterpret_cast<const ut8 *>(bytes.constData()), bytes.size(), hex.data());
    return QString::fromUtf8(hex);
}

void CutterCore::loadScript(const QString &scriptname)
{
    r_core_task_sync_begin(core_);
    r_core_cmd_file(core_, scriptname.toUtf8().constData());
    r_core_task_sync_end(core_);
    triggerRefreshAll();
}

QString CutterCore::getVersionInformation()
{
    int i;
    QString versionInfo;
    struct vcs_t {
        const char *name;
        const char *(*callback)();
    } vcs[] = {
        { "r_anal", &r_anal_version },
        { "r_lib", &r_lib_version },
        { "r_egg", &r_egg_version },
        { "r_asm", &r_asm_version },
        { "r_bin", &r_bin_version },
        { "r_cons", &r_cons_version },
        { "r_flag", &r_flag_version },
        { "r_core", &r_core_version },
        { "r_crypto", &r_crypto_version },
        { "r_bp", &r_bp_version },
        { "r_debug", &r_debug_version },
        { "r_hash", &r_hash_version },
        { "r_fs", &r_fs_version },
        { "r_io", &r_io_version },
#if !USE_LIB_MAGIC        
        { "r_magic", &r_magic_version },
#endif
        { "r_parse", &r_parse_version },
        { "r_reg", &r_reg_version },
        { "r_sign", &r_sign_version },
        { "r_search", &r_search_version },
        { "r_syscall", &r_syscall_version },
        { "r_util", &r_util_version },
        /* ... */
        {NULL, NULL}
    };
    versionInfo.append(QString("%1 r2\n").arg(R2_GITTAP));
    for (i = 0; vcs[i].name; i++) {
        struct vcs_t *v = &vcs[i];
        const char *name = v->callback ();
        versionInfo.append(QString("%1 %2\n").arg(name, v->name));
    }
    return versionInfo;
}

QJsonArray CutterCore::getOpenedFiles()
{
    QJsonDocument files = cmdj("oj");
    return files.array();
}

QList<QString> CutterCore::getColorThemes()
{
    QList<QString> r;
    QJsonDocument themes = cmdj("ecoj");
    for (const QJsonValue &s : themes.array()) {
        r << s.toString();
    }
    return r;
}

QString CutterCore::ansiEscapeToHtml(const QString &text)
{
    int len;
    char *html = r_cons_html_filter(text.toUtf8().constData(), &len);
    if (!html) {
        return QString();
    }
    QString r = QString::fromUtf8(html, len);
    r_free(html);
    return r;
}

BasicBlockHighlighter* CutterCore::getBBHighlighter()
{
    return bbHighlighter;
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
            tempConfig
                .set("scr.color", COLOR_MODE_16M)
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
    tempConfig
        .set("scr.color", COLOR_MODE_16M)
        .set("asm.offset", true)
        .set("hex.header", false)
        .set("hex.cols", 16);
    return ansiEscapeToHtml(hexdump(address, size, HexdumpFormats::Normal)).replace(QLatin1Char('\n'), "<br>");
}

QByteArray CutterCore::ioRead(RVA addr, int len)
{
    CORE_LOCK();

    QByteArray array;

    if (len <= 0)
        return array;

    /* Zero-copy */
    array.resize(len);
    if (!r_io_read_at(core_->io, addr, (uint8_t *)array.data(), len)) {
        qWarning() << "Can't read data" << addr << len;
        array.fill(0xff);
    }

    return  array;
}

