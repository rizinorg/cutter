#include <QJsonArray>
#include <QJsonObject>
#include <utils/TempConfig.h>
#include "utils/Configuration.h"
#include "cutter.h"
#include "sdb.h"

Q_GLOBAL_STATIC(ccClass, uniqueInstance)

#define DB this->db

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

CutterCore::CutterCore(QObject *parent) :
    QObject(parent)
{
    r_cons_new();  // initialize console
    this->core_ = r_core_new();
    r_core_loadlibs(this->core_, R_CORE_LOADLIBS_ALL, NULL);
    // IMPLICIT r_bin_iobind (core_->bin, core_->io);

    // Otherwise r2 may ask the user for input and Cutter would freeze
    setConfig("scr.interactive", false);

    // Used by the HTML5 graph
    setConfig("http.cors", true);
    setConfig("http.sandbox", false);
    //config("http.port", "14170");

    // Temporary fixes
    //config("http.root","/usr/local/share/radare2/last/www");
    //config("http.root","/usr/local/radare2/osx/share/radare2/1.1.0-git/www");

    default_bits = 0;

    this->db = sdb_new(NULL, NULL, 0);  // WTF NOES
}


CutterCore *CutterCore::getInstance()
{
    return uniqueInstance;
}

QList<QString> CutterCore::sdbList(QString path)
{
    CORE_LOCK();
    QList<QString> list = QList<QString>();
    Sdb *root = sdb_ns_path(core_->sdb, path.toUtf8().constData(), 0);
    if (root)
    {
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

QList<QString> CutterCore::sdbListKeys(QString path)
{
    CORE_LOCK();
    QList<QString> list = QList<QString>();
    Sdb *root = sdb_ns_path(core_->sdb, path.toUtf8().constData(), 0);
    if (root)
    {
        void *vsi;
        ls_iter_t *iter;
        SdbList *l = sdb_foreach_list(root, false);
        ls_foreach(l, iter, vsi)
        {
            SdbKv *nsi = (SdbKv *)vsi;
            list << nsi->key;
        }
    }
    return list;
}

QString CutterCore::sdbGet(QString path, QString key)
{
    CORE_LOCK();
    Sdb *db = sdb_ns_path(core_->sdb, path.toUtf8().constData(), 0);
    if (db)
    {
        const char *val = sdb_const_get(db, key.toUtf8().constData(), 0);
        if (val && *val)
            return val;
    }
    return QString("");
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
    r_core_free(this->core_);
    r_cons_free();
}

QString CutterCore::sanitizeStringForCommand(QString s)
{
    static const QRegExp regexp(";|@");
    return s.replace(regexp, "_");
}

QString CutterCore::cmd(const QString &str)
{
    CORE_LOCK();

    RVA offset = core_->offset;
    QByteArray cmd = str.toUtf8();
    char *res = r_core_cmd_str(this->core_, cmd.constData());
    QString o = QString(res ? res : "");
    r_mem_free(res);
    if (offset != core_->offset)
    {
        emit seekChanged(core_->offset);

        // switch from graph to disassembly if there is no function
        if (this->cmd("afi.").trimmed().isEmpty() && memoryWidgetPriority == MemoryWidgetType::Graph)
        {
            memoryWidgetPriority = MemoryWidgetType::Disassembly;
        }

        triggerRaisePrioritizedMemoryWidget();
    }
    return o;
}

QString CutterCore::cmdRaw(const QString &str)
{
    QString cmdStr = str;
    cmdStr.replace('\"', "\\\"");
    return cmd("\"" + cmdStr + "\"");
}

QJsonDocument CutterCore::cmdj(const QString &str)
{
    CORE_LOCK();
    QByteArray cmd = str.toUtf8();

    char *res = r_core_cmd_str(this->core_, cmd.constData());

    QString resString = QString(res);

    if (resString.isEmpty())
    {
        r_mem_free(res);
        return QJsonDocument();
    }

    QJsonParseError jsonError;
    QJsonDocument doc = res ? QJsonDocument::fromJson(resString.toUtf8(), &jsonError) : QJsonDocument();

    if (jsonError.error != QJsonParseError::NoError)
    {
        eprintf("Failed to parse JSON for command \"%s\": %s\n", str.toLocal8Bit().constData(), jsonError.errorString().toLocal8Bit().constData());
        eprintf("%s\n", resString.toLocal8Bit().constData());
    }

    r_mem_free(res);
    return doc;
}

bool CutterCore::loadFile(QString path, uint64_t loadaddr, uint64_t mapaddr, bool rw, int va, int idx, bool loadbin, const QString &forceBinPlugin)
{
    Q_UNUSED(loadaddr);
    Q_UNUSED(idx);

    CORE_LOCK();
    RCoreFile *f;
    if (va == 0 || va == 2)
        r_config_set_i(core_->config, "io.va", va);

    f = r_core_file_open(core_, path.toUtf8().constData(), rw ? (R_IO_READ | R_IO_WRITE) : R_IO_READ, mapaddr);
    if (!f)
    {
        eprintf("r_core_file_open failed\n");
        return false;
    }

    if (!forceBinPlugin.isNull())
    {
        r_bin_force_plugin(r_core_get_bin(core_), forceBinPlugin.toUtf8().constData());
    }

    if (loadbin)
    {
        if (va == 1)
        {
            if (!r_core_bin_load(core_, path.toUtf8().constData(), UT64_MAX))
            {
                eprintf("CANNOT GET RBIN INFO\n");
            }
        }
        else
        {
            if (!r_core_bin_load(core_, path.toUtf8().constData(), UT64_MAX))
            {
                eprintf("CANNOT GET RBIN INFO\n");
            }
        }

#if HAVE_MULTIPLE_RBIN_FILES_INSIDE_SELECT_WHICH_ONE
        if (!r_core_file_open(core, path.toUtf8(), R_IO_READ | (rw ? R_IO_WRITE : 0, mapaddr)))
        {
            eprintf("Cannot open file\n");
        }
        else
        {
            // load RBin information
            // XXX only for sub-bins
            r_core_bin_load(core, path.toUtf8(), loadaddr);
            r_bin_select_idx(core_->bin, NULL, idx);
        }
#endif
    }
    else
    {
        // Not loading RBin info coz va = false
    }

    setDefaultCPU();

    r_core_hash_load(core_, path.toUtf8().constData());
    fflush(stdout);
    return true;
}

void CutterCore::analyze(int level,  QList<QString> advanced)
{
    CORE_LOCK();
    /*
     * Levels
     * Nivel 1: aaa
     * Nivel 2: aaaa
     */

    if (level == 1)
    {
        r_core_cmd0(core_, "aaa");
    }
    else if (level == 2)
    {
        r_core_cmd0(core_, "aaaa");
    }
    else if (level == 3)
    {
        foreach (QString option, advanced)
        {
            r_core_cmd0(core_, option.toStdString().c_str());
        }
    }
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
    if (offset == RVA_INVALID)
    {
        offset = getOffset();
    }

    this->cmd("ahi " + r2BaseName + " @ " + QString::number(offset));
    emit instructionChanged(offset);
}

void CutterCore::seek(QString addr)
{
    // Slower than using the API, but the API is not complete
    // which means we either have to duplicate code from radare2
    // here, or refactor radare2 API.
    CORE_LOCK();
    cmd(QString("s %1").arg(addr));
    // cmd already does emit seekChanged(core_->offset);
}

void CutterCore::seek(ut64 offset)
{
    seek(QString::number(offset));
}

void CutterCore::seekPrev()
{
    cmd("s-");
}

void CutterCore::seekNext()
{
    cmd("s+");
}

RVA CutterCore::prevOpAddr(RVA startAddr, int count)
{
    CORE_LOCK();
    bool ok;
    RVA offset = cmd("/O " + QString::number(count) + " @ " + QString::number(startAddr)).toULongLong(&ok, 16);
    return ok ? offset : startAddr - count;
}

RVA CutterCore::nextOpAddr(RVA startAddr, int count)
{
    CORE_LOCK();

    QJsonArray array = Core()->cmdj("pdj " + QString::number(count+1) + "@" + QString::number(startAddr)).array();
    if (array.isEmpty())
    {
        return startAddr + 1;
    }

    QJsonValue instValue = array.last();
    if (!instValue.isObject())
    {
        return startAddr + 1;
    }

    bool ok;
    RVA offset = instValue.toObject()["offset"].toVariant().toULongLong(&ok);
    if (!ok)
    {
        return startAddr + 1;
    }

    return offset;
}

RVA CutterCore::getOffset()
{
    return core_->offset;
}

bool CutterCore::tryFile(QString path, bool rw)
{
    CORE_LOCK();
    RCoreFile *cf;
    int flags = R_IO_READ;
    if (rw) flags |= R_IO_WRITE;
    cf = r_core_file_open(this->core_, path.toUtf8().constData(), flags, 0LL);
    if (!cf)
    {
        eprintf("QRCore::tryFile: Cannot open file?\n");
        return false;
    }

    bool is_writable = false;
    if (cf->core && cf->core->io && cf->core->io->desc)
    {
        is_writable = cf->core->io->desc->flags & R_IO_WRITE;
    }
    // if rbin works, tell entry0, and symbols (main, constructor, ..)

    r_core_file_close (this->core_, cf);

    sdb_bool_set(DB, "try.is_writable", is_writable, 0);
    sdb_set(DB, "try.filetype", "elf.i386", 0);
    sdb_set(DB, "try.filename", path.toUtf8().constData(), 0);
    return true;
}

ut64 CutterCore::math(const QString &expr)
{
    CORE_LOCK();
    return r_num_math(this->core_ ? this->core_->num : NULL, expr.toUtf8().constData());
}

QString CutterCore::itoa(ut64 num, int rdx)
{
    return QString::number(num, rdx);
}

void CutterCore::setConfig(const QString &k, const QString &v)
{
    CORE_LOCK();
    r_config_set(core_->config, k.toUtf8().constData(), v.toUtf8().constData());
}

void CutterCore::setConfig(const QString &k, int v)
{
    CORE_LOCK();
    r_config_set_i(core_->config, k.toUtf8().constData(), static_cast<const unsigned long long int>(v));
}

void CutterCore::setConfig(const QString &k, bool v)
{
    CORE_LOCK();
    r_config_set_i(core_->config, k.toUtf8().constData(), v ? 1 : 0);
}

int CutterCore::getConfigi(const QString &k)
{
    CORE_LOCK();
    QByteArray key = k.toUtf8();
    return static_cast<int>(r_config_get_i(core_->config, key.constData()));
}

bool CutterCore::getConfigb(const QString &k)
{
    CORE_LOCK();
    return r_config_get_i(core_->config, k.toUtf8().constData()) != 0;
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

void CutterCore::resetDefaultAsmOptions()
{
    // TODO Merge with Configuration.cpp
    setConfig("asm.esil", Config()->getAsmESIL());
    setConfig("asm.pseudo", Config()->getAsmPseudo());
    setConfig("asm.offset", Config()->getAsmOffset());
    setConfig("asm.describe", Config()->getAsmDescribe());
    setConfig("asm.stackptr", Config()->getAsmStackPointer());
    setConfig("asm.bytes", Config()->getAsmBytes());
    setConfig("asm.bytespace", Config()->getAsmBytespace());
    setConfig("asm.lbytes", Config()->getAsmLBytes());
    setConfig("asm.syntax", Config()->getAsmSyntax());
    setConfig("asm.ucase", Config()->getAsmUppercase());
    setConfig("asm.bbline", Config()->getAsmBBLine());
    setConfig("asm.capitalize", Config()->getAsmCapitalize());
    setConfig("asm.varsub", Config()->getAsmVarsub());
    setConfig("asm.varsub_only", Config()->getAsmVarsubOnly());
}

void CutterCore::saveDefaultAsmOptions()
{
    Config()->setAsmESIL(getConfigb("asm.esil"));
    Config()->setAsmPseudo(getConfigb("asm.pseudo"));
    Config()->setAsmOffset(getConfigb("asm.offset"));
    Config()->setAsmDescribe(getConfigb("asm.describe"));
    Config()->setAsmStackPointer(getConfigb("asm.stackptr"));
    Config()->setAsmBytes(getConfigb("asm.bytes"));
    Config()->setAsmBytespace(getConfigb("asm.bytespace"));
    Config()->setAsmLBytes(getConfigb("asm.lbytes"));
    Config()->setAsmSyntax(getConfig("asm.syntax"));
    Config()->setAsmUppercase(getConfigb("asm.ucase"));
    Config()->setAsmBBLine(getConfigb("asm.bbline"));
    Config()->setAsmCapitalize(getConfigb("asm.capitalize"));
    Config()->setAsmVarsub(getConfigb("asm.varsub"));
    Config()->setAsmVarsubOnly(getConfigb("asm.varsub_only"));
}

QString CutterCore::getConfig(const QString &k)
{
    CORE_LOCK();
    QByteArray key = k.toUtf8();
    return QString(r_config_get(core_->config, key.constData()));
}

void CutterCore::setOptions(QString key)
{
    Q_UNUSED(key);

    // va
    // lowercase
    // show bytes
    // att syntax
    // asm plugin
    // cpu type
    // anal plugin
}

void CutterCore::setCPU(QString arch, QString cpu, int bits, bool temporary)
{
    setConfig("asm.arch", arch);
    setConfig("asm.cpu", cpu);
    setConfig("asm.bits", bits);
    if (!temporary)
    {
        default_arch = arch;
        default_cpu = cpu;
        default_bits = bits;
    }
}

void CutterCore::setDefaultCPU()
{
    if (!default_arch.isEmpty())
        setConfig("asm.arch", default_arch);
    if (!default_cpu.isEmpty())
        setConfig("asm.cpu", default_cpu);
    if (default_bits)
        setConfig("asm.bits", QString::number(default_bits));
}

QString CutterCore::assemble(const QString &code)
{
    CORE_LOCK();
    RAsmCode *ac = r_asm_massemble(core_->assembler, code.toUtf8().constData());
    QString hex(ac != nullptr ? ac->buf_hex : "");
    r_asm_code_free(ac);
    return hex;
}

QString CutterCore::disassemble(const QString &hex)
{
    CORE_LOCK();
    RAsmCode *ac = r_asm_mdisassemble_hexstr(core_->assembler, hex.toUtf8().constData());
    QString code = QString(ac != nullptr ? ac->buf_asm : "");
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
    //return r_anal_fcn_find (core_->anal, addr, addr);
    return r_anal_get_fcn_in(core_->anal, addr, 0);
}

QString CutterCore::cmdFunctionAt(QString addr)
{
    QString ret;
    //afi~name:1[1] @ 0x08048e44
    //ret = cmd("afi~name[1] @ " + addr);
    ret = cmd(QString("fd @ ") + addr + "~[0]");
    return ret.trimmed();
}

QString CutterCore::cmdFunctionAt(RVA addr)
{
    return cmdFunctionAt(QString::number(addr));
}

QString CutterCore::createFunctionAt(RVA addr, QString name)
{
    name.remove(QRegExp("[^a-zA-Z0-9_]"));
    QString command = "af " + name + " " + RAddressString(addr);
    QString ret = cmd(command);
    emit functionsChanged();
    return ret;
}

void CutterCore::markString(RVA addr)
{
    cmd("Cs @" + RAddressString(addr));
}

int CutterCore::get_size()
{
    CORE_LOCK();
    RBinObject *obj = r_bin_get_object(core_->bin);
    //return obj->size;
    return obj != nullptr ? obj->obj_size : 0;
}

ulong CutterCore::get_baddr()
{
    CORE_LOCK();
    ulong baddr = r_bin_get_baddr(core_->bin);
    return baddr;
}

QList<QList<QString>> CutterCore::get_exec_sections()
{
    QList<QList<QString>> ret;

    QString text = cmd("S*~^S");
    for (QString line : text.split("\n"))
    {
        QStringList fields = line.split(" ");
        if (fields.length() == 7)
        {
            if (fields[6].contains("x"))
            {
                QList<QString> tmp = QList<QString>();
                tmp << fields[2];
                tmp << fields[3];
                tmp << fields[5];
                ret << tmp;
            }
        }
    }
    return ret;
}

QString CutterCore::getOffsetInfo(QString addr)
{
    return cmd("ao @ " + addr);
}

RVA CutterCore::getOffsetJump(RVA addr)
{
    bool ok;
    RVA value = cmdj("aoj @" + QString::number(addr)).array().first().toObject().value("jump").toVariant().toULongLong(&ok);

    if (!ok)
    {
        return RVA_INVALID;
    }

    return value;
}

QString CutterCore::getDecompiledCode(RVA addr)
{
    return cmd("pdc @ " + QString::number(addr));
}

QString CutterCore::getDecompiledCode(QString addr)
{
    return cmd("pdc @ " + addr);
}

QString CutterCore::getFileInfo()
{
    QString info = cmd("ij");
    return info;
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

QString CutterCore::getSimpleGraph(QString function)
{
    // New styles
    QString graph = "graph [bgcolor=invis, splines=polyline];";
    QString node = "node [style=\"filled\" fillcolor=\"#4183D7\" shape=box fontname=\"Courier\" fontsize=\"8\" color=\"#4183D7\" fontcolor=\"white\"];";
    QString arrow = "edge [arrowhead=\"normal\";]";

    // Old styles
    QString old_graph = "graph [bgcolor=white fontsize=8 fontname=\"Courier\"];";
    //QString old_node = "node [color=lightgray, style=filled shape=box];";
    QString old_node = "node [fillcolor=gray style=filled shape=box];";
    QString old_arrow = "edge [arrowhead=\"vee\"];";

    QString dot = cmd("aga @ " + function).trimmed();
    dot.replace(old_graph, graph);
    dot.replace(old_node, node);
    dot.replace(old_arrow, arrow);
    dot.replace("fillcolor=blue", "fillcolor=\"#EC644B\", color=\"#EC644B\"");

    return dot;
}

void CutterCore::getOpcodes()
{
    QString opcodes = cmd("?O");
    this->opcodes = opcodes.split("\n");
    // Remove the last empty element
    this->opcodes.removeLast();
    QString registers = cmd("drp~[1]");
    this->regs = registers.split("\n");
    this->regs.removeLast();
}

void CutterCore::setSettings()
{
    setConfig("scr.interactive", false);
    setConfig("asm.lines", false);
    // Intredazting...
    //setConfig("asm.linesright", "true");
    //setConfig("asm.lineswidth", "15");
    //setConfig("asm.functions", "false");
    setConfig("hex.pairs", false);
    setConfig("asm.cmtflgrefs", false);
    setConfig("asm.cmtright", true);
    setConfig("asm.cmtcol", 70);
    setConfig("asm.xrefs", false);
    setConfig("asm.fcnlines", false);

    setConfig("asm.tabs", 5);
    setConfig("asm.tabsonce", true);
    setConfig("asm.tabsoff", 5);
    setConfig("asm.nbytes", 10);
    setConfig("asm.midflags", 2);
    //setConfig("asm.bbline", "true");

    // asm.offset=false would break reading the offset in DisassemblyWidget
    // TODO: remove this when DisassemblyWidget::readDisassemblyOffset() allows it
    setConfig("asm.offset", true);

    setConfig("anal.hasnext", true);
    setConfig("asm.fcncalls", false);
    setConfig("asm.calls", false);
    setConfig("asm.lines.call", false);
    setConfig("asm.flgoff", true);
    setConfig("anal.autoname", true);

    // Highlight current node in graphviz
    setConfig("graph.gv.current", true);

    // Fucking pancake xD
    setConfig("cfg.fortunes.tts", false);

    // Experimenting with asm options
    //setConfig("asm.spacy", "true");      // We need to handle blank lines on scroll
    //setConfig("asm.section", "true");    // Breaks the disasm and navigation
    //setConfig("asm.invhex", "true");     // Needs further testing
    //setConfig("asm.flags", "false");     // Add with default true in future

    // Used by the HTML5 graph
    setConfig("http.cors", true);
    setConfig("http.sandbox", false);
    //config("http.port", "14170");

    // Temporary fixes
    //setConfig("http.root","/usr/local/share/radare2/last/www");
    //setConfig("http.root","/usr/local/radare2/osx/share/radare2/1.1.0-git/www");
    //setConfig("bin.rawstr", "true");

    // Colors
    setConfig("scr.color", false);
    setConfig("scr.truecolor", false);
}

QList<RVA> CutterCore::getSeekHistory()
{
    CORE_LOCK();
    QList<RVA> ret;

    QJsonArray jsonArray = cmdj("sj").array();
    foreach (QJsonValue value, jsonArray)
        ret << value.toVariant().toULongLong();

    return ret;
}

QStringList CutterCore::getAsmPluginNames()
{
    CORE_LOCK();
    RListIter *it;
    QStringList ret;

    RAsmPlugin *ap;
    CutterRListForeach(core_->assembler->plugins, it, RAsmPlugin, ap)
    {
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
    CutterRListForeach(core_->anal->plugins, it, RAnalPlugin, ap)
    {
        ret << ap->name;
    }

    return ret;
}

QStringList CutterCore::getProjectNames()
{
    CORE_LOCK();
    QStringList ret;

    QJsonArray jsonArray = cmdj("Plj").array();
    for (QJsonValue value : jsonArray)
        ret.append(value.toString());

    return ret;
}

QList<RBinPluginDescription> CutterCore::getRBinPluginDescriptions(const QString &type)
{
    QList<RBinPluginDescription> ret;

    QJsonObject jsonRoot = cmdj("iLj").object();
    for (const QString &key : jsonRoot.keys())
    {
        if (!type.isNull() && key != type)
            continue;

        QJsonArray pluginArray = jsonRoot[key].toArray();

        for (const auto &pluginValue : pluginArray)
        {
            QJsonObject pluginObject = pluginValue.toObject();
            RBinPluginDescription desc;
            desc.name = pluginObject["name"].toString();
            desc.description = pluginObject["description"].toString();
            desc.license = pluginObject["license"].toString();
            desc.type = key;
            ret.append(desc);
        }
    }

    return ret;
}

QList<FunctionDescription> CutterCore::getAllFunctions()
{
    CORE_LOCK();
    QList<FunctionDescription> ret;

    QJsonArray jsonArray = cmdj("aflj").array();

    foreach (QJsonValue value, jsonArray)
    {
        QJsonObject jsonObject = value.toObject();

        FunctionDescription function;

        function.offset = (RVA)jsonObject["offset"].toVariant().toULongLong();
        function.size = (RVA)jsonObject["size"].toVariant().toULongLong();
        function.name = jsonObject["name"].toString();

        ret << function;
    }

    return ret;
}

QList<ImportDescription> CutterCore::getAllImports()
{
    CORE_LOCK();
    QList<ImportDescription> ret;

    QJsonArray importsArray = cmdj("iij").array();

    foreach (QJsonValue value, importsArray)
    {
        QJsonObject importObject = value.toObject();

        ImportDescription import;

        import.plt = importObject["plt"].toVariant().toULongLong();
        import.ordinal = importObject["ordinal"].toInt();
        import.bind = importObject["bind"].toString();
        import.type = importObject["type"].toString();
        import.name = importObject["name"].toString();

        ret << import;
    }

    return ret;
}

QList<ExportDescription> CutterCore::getAllExports()
{
    CORE_LOCK();
    QList<ExportDescription> ret;

    QJsonArray importsArray = cmdj("iEj").array();

    foreach (QJsonValue value, importsArray)
    {
        QJsonObject importObject = value.toObject();

        ExportDescription exp;

        exp.vaddr = importObject["vaddr"].toVariant().toULongLong();
        exp.paddr = importObject["paddr"].toVariant().toULongLong();
        exp.size = importObject["size"].toVariant().toULongLong();
        exp.type = importObject["type"].toString();
        exp.name = importObject["name"].toString();
        exp.flag_name = importObject["flagname"].toString();

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
    if (core_ && core_->bin && core_->bin->cur && core_->bin->cur->o)
    {
        CutterRListForeach(core_->bin->cur->o->symbols, it, RBinSymbol, bs)
        {
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
        CutterRListForeach(core_->bin->cur->o->entries, it, RBinAddr, entry)
        {
            SymbolDescription symbol;
            symbol.vaddr = entry->vaddr;
            symbol.name = QString("entry") + QString::number(n++);
            symbol.bind = "";
            symbol.type = "entry";
            ret << symbol;
        }
    }

    return ret;
}

QList<CommentDescription> CutterCore::getAllComments(const QString &filterType)
{
    CORE_LOCK();
    QList<CommentDescription> ret;

    QJsonArray commentsArray = cmdj("CCj").array();
    for (QJsonValue value : commentsArray)
    {
        QJsonObject commentObject = value.toObject();

        QString type = commentObject["type"].toString();
        if (type != filterType)
            continue;

        CommentDescription comment;
        comment.offset = commentObject["offset"].toVariant().toULongLong();
        comment.name = commentObject["name"].toString();

        ret << comment;
    }
    return ret;
}

QList<RelocDescription> CutterCore::getAllRelocs()
{
    CORE_LOCK();
    RListIter *it;
    QList<RelocDescription> ret;

    RBinReloc *br;
    if (core_ && core_->bin && core_->bin->cur && core_->bin->cur->o)
    {
        CutterRListForeach(core_->bin->cur->o->relocs, it, RBinReloc, br)
        {
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
    CORE_LOCK();
    RListIter *it;
    QList<StringDescription> ret;

    RBinString *bs;
    if (core_ && core_->bin && core_->bin->cur && core_->bin->cur->o)
    {
        CutterRListForeach(core_->bin->cur->o->strings, it, RBinString, bs)
        {
            StringDescription str;
            str.vaddr = bs->vaddr;
            str.string = bs->string;
            ret << str;
        }
    }

    return ret;
}

QList<FlagspaceDescription> CutterCore::getAllFlagspaces()
{
    CORE_LOCK();
    QList<FlagspaceDescription> ret;

    QJsonArray flagspacesArray = cmdj("fsj").array();
    for (QJsonValue value : flagspacesArray)
    {
        QJsonObject flagspaceObject = value.toObject();

        FlagspaceDescription flagspace;
        flagspace.name = flagspaceObject["name"].toString();

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
    for (QJsonValue value : flagsArray)
    {
        QJsonObject flagObject = value.toObject();

        FlagDescription flag;
        flag.offset = flagObject["offset"].toVariant().toULongLong();
        flag.size = flagObject["size"].toVariant().toULongLong();
        flag.name = flagObject["name"].toString();

        ret << flag;
    }
    return ret;
}

QList<SectionDescription> CutterCore::getAllSections()
{
    CORE_LOCK();
    QList<SectionDescription> ret;

    QJsonArray sectionsArray = cmdj("Sj").array();
    for (QJsonValue value : sectionsArray)
    {
        QJsonObject sectionObject = value.toObject();

        QString name = sectionObject["name"].toString();
        if (name.isEmpty())
            continue;

        SectionDescription section;
        section.name = name;
        section.vaddr = sectionObject["vaddr"].toVariant().toULongLong();
        section.vsize = sectionObject["vsize"].toVariant().toULongLong();
        section.paddr = sectionObject["paddr"].toVariant().toULongLong();
        section.size = sectionObject["size"].toVariant().toULongLong();
        section.flags = sectionObject["flags"].toString();

        ret << section;
    }
    return ret;
}

QList<EntrypointDescription> CutterCore::getAllEntrypoint()
{
    CORE_LOCK();
    QList<EntrypointDescription> ret;

    QJsonArray entrypointsArray = cmdj("iej").array();
    for (QJsonValue value : entrypointsArray)
    {
        QJsonObject entrypointObject = value.toObject();

        EntrypointDescription entrypoint;
        entrypoint.vaddr = entrypointObject["vaddr"].toVariant().toULongLong();
        entrypoint.paddr = entrypointObject["paddr"].toVariant().toULongLong();
        entrypoint.baddr = entrypointObject["baddr"].toVariant().toULongLong();
        entrypoint.laddr = entrypointObject["laddr"].toVariant().toULongLong();
        entrypoint.haddr = entrypointObject["haddr"].toVariant().toULongLong();
        entrypoint.type = entrypointObject["type"].toString();

        ret << entrypoint;
    }
    return ret;
}

QList<ClassDescription> CutterCore::getAllClasses()
{
    CORE_LOCK();
    QList<ClassDescription> ret;

    QJsonArray classesArray = cmdj("icj").array();
    for (QJsonValueRef value : classesArray)
    {
        QJsonObject classObject = value.toObject();

        ClassDescription cls;
        cls.name = classObject["classname"].toString();
        cls.addr = classObject["addr"].toVariant().toULongLong();
        cls.index = classObject["index"].toVariant().toULongLong();

        for(QJsonValueRef value2 : classObject["methods"].toArray())
        {
            QJsonObject methObject = value2.toObject();

            ClassMethodDescription meth;
            meth.name = methObject["name"].toString();
            meth.addr = methObject["addr"].toVariant().toULongLong();
            cls.methods << meth;
        }

        for(QJsonValueRef value2 : classObject["fields"].toArray())
        {
            QJsonObject fieldObject = value2.toObject();

            ClassFieldDescription field;
            field.name = fieldObject["name"].toString();
            field.addr = fieldObject["addr"].toVariant().toULongLong();
            cls.fields << field;
        }

        ret << cls;
    }
    return ret;
}

QList<XrefDescription> CutterCore::getXRefs(RVA addr, bool to, bool whole_function, const QString &filterType)
{
    QList<XrefDescription> ret = QList<XrefDescription>();

    QJsonArray xrefsArray;

    if (to)
        xrefsArray = cmdj("axtj@" + QString::number(addr)).array();
    else
        xrefsArray = cmdj("axfj@" + QString::number(addr)).array();

    for (QJsonValue value : xrefsArray)
    {
        QJsonObject xrefObject = value.toObject();

        XrefDescription xref;
        xref.type = xrefObject["type"].toString();

        if (!filterType.isNull() && filterType != xref.type)
            continue;

        xref.from = xrefObject["from"].toVariant().toULongLong();
        xref.from_str = Core()->cmd("fd " + xrefObject["from"].toString()).trimmed();

        if (!whole_function && !to && xref.from != addr)
            continue;

        if (to && !xrefObject.contains("to"))
            xref.to = addr;
        else
            xref.to = xrefObject["to"].toVariant().toULongLong();
        xref.to_str = Core()->cmd("fd " + xrefObject["to"].toString()).trimmed();

        ret << xref;
    }

    return ret;
}

void CutterCore::addFlag(RVA offset, QString name, RVA size)
{
    name = sanitizeStringForCommand(name);
    cmd(QString("f %1 %2 @ %3").arg(name).arg(size).arg(offset));
    emit flagsChanged();
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
    setNotes(notes);
}

void CutterCore::saveProject(const QString &name)
{
    cmd("Ps " + name);
    cmd("Pnj " + notes.toUtf8().toBase64());
    emit projectSaved(name);
}

bool CutterCore::isProjectNameValid(const QString &name)
{
    // see is_valid_project_name() in libr/core/project.c
    static const QRegExp regexp(R"(^[a-zA-Z0-9\\\._:-]{1,}$)");
    return regexp.exactMatch(name) && !name.endsWith(".zip") ;
}

void CutterCore::setNotes(const QString &notes)
{
    this->notes = notes;
    emit notesChanged(this->notes);
}

QList<DisassemblyLine> CutterCore::disassembleLines(RVA offset, int lines)
{
    QJsonArray array = cmdj(QString("pdJ ") + QString::number(lines) + QString(" @ ") + QString::number(offset)).array();
    QList<DisassemblyLine> r;

    for (QJsonValue value : array)
    {
        QJsonObject object = value.toObject();

        DisassemblyLine line;
        line.offset = object["offset"].toVariant().toULongLong();
        line.text = object["text"].toString();

        r << line;
    }

    return r;
}

void CutterCore::loadScript(const QString &scriptname)
{
    r_core_cmd_file(core_, scriptname.toStdString().data());
}

QString CutterCore::getVersionInformation()
{
    int i;
    QString ret;
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
        { "r_magic", &r_magic_version },
        { "r_parse", &r_parse_version },
        { "r_reg", &r_reg_version },
        { "r_sign", &r_sign_version },
        { "r_search", &r_search_version },
        { "r_syscall", &r_syscall_version },
        { "r_util", &r_util_version },
        /* ... */
        {NULL,NULL}
    };
    ret.append(QString("%1 r2\n").arg(R2_GITTAP));
    for (i = 0; vcs[i].name; i++) {
            struct vcs_t *v = &vcs[i];
            const char *name = v->callback ();
            ret.append(QString("%1 %2\n").arg(name, v->name));
    }
    return ret;
}
