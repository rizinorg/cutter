#include <QJsonArray>
#include <QJsonObject>
#include "cutter.h"
#include "sdb.h"
#include "Settings.h"

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
    this->projectPath = "";
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


int CutterCore::getCycloComplex(ut64 addr)
{
    CORE_LOCK();
    QString ret = "";
    RAnalFunction *fcn = r_anal_get_fcn_in(core_->anal, addr, 0);
    if (fcn)
    {
        ret = cmd("afcc @ " + QString(fcn->name));
        return ret.toInt();
    }
    else
    {
        eprintf("qcore->getCycloComplex: no fcn found");
        return 0;
    }
}

int CutterCore::getFcnSize(ut64 addr)
{
    CORE_LOCK();
    QString ret = "";
    QString tmp_ret = "";
    RAnalFunction *fcn = r_anal_get_fcn_in(core_->anal, addr, 0);
    if (fcn)
    {
        tmp_ret = cmd("afi~size[1] " + QString(fcn->name));
        ret = tmp_ret.split("\n")[0];
        return ret.toInt() / 10;
    }
    else
    {
        eprintf("qcore->getFcnSize: no fcn found");
        return 0;
    }
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
    if (offset != core_->offset) {
        emit seekChanged(core_->offset);
    }
    return o;
}

QJsonDocument CutterCore::cmdj(const QString &str)
{
    CORE_LOCK();
    QByteArray cmd = str.toUtf8();

    char *res = r_core_cmd_str(this->core_, cmd.constData());

    QString resString = QString(res);

    QJsonParseError jsonError;
    QJsonDocument doc = res ? QJsonDocument::fromJson(resString.toUtf8(), &jsonError) : QJsonDocument();

    if (jsonError.error != QJsonParseError::NoError)
    {
        eprintf("Failed to parse JSON: %s\n", jsonError.errorString().toLocal8Bit().constData());
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
    // NO ONE KNOWS WHY THIS IS FIXING A SEGFAULT. core_->file should have already a proper value. Pancake dixit
    //core_->file = NULL;
    // mapaddr = 0LL;
    printf("FILE OPEN (%s)\n", path.toUtf8().constData());
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
            if (r_core_bin_load(core_, path.toUtf8().constData(), UT64_MAX))
            {
                RBinObject *obj = r_bin_get_object(core_->bin);
                if (obj)
                {
                    eprintf("BITS %d\n", obj->info->bits);
                }
            }
            else
            {
                eprintf("CANNOT GET RBIN INFO\n");
            }
        }
        else
        {
            if (r_core_bin_load(core_, path.toUtf8().constData(), UT64_MAX))
            {
                RBinObject *obj = r_bin_get_object(core_->bin);
                if (obj)
                {
                    eprintf("BITS %d\n", obj->info->bits);
                }
                else
                {
                    eprintf("Bin load failed\n");
                    return false;
                }
            }
            else
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

void CutterCore::renameFunction(QString prev_name, QString new_name)
{
    cmd("afn " + new_name + " " + prev_name);
    emit functionRenamed(prev_name, new_name);
}


void CutterCore::setComment(RVA addr, QString cmt)
{
    //r_meta_add (core->anal, 'C', addr, 1, cmt.toUtf8());
    cmd("CC " + cmt + " @ " + QString::number(addr));
    emit commentsChanged();
}

void CutterCore::delComment(ut64 addr)
{
    CORE_LOCK();
    r_meta_del(core_->anal, 'C', addr, 1, NULL);
    //cmd (QString("CC-@")+addr);
}

QMap<QString, QList<QList<QString>>> CutterCore::getNestedComments()
{
    QMap<QString, QList<QList<QString>>> ret;
    QString comments = cmd("CC~CCu");

    for (QString line : comments.split("\n"))
    {
        QStringList fields = line.split("CCu");
        if (fields.length() == 2)
        {
            QList<QString> tmp = QList<QString>();
            tmp << fields[1].split("\"")[1].trimmed();
            tmp << fields[0].trimmed();
            QString fcn_name = this->cmdFunctionAt(fields[0].trimmed());
            ret[fcn_name].append(tmp);
        }
    }
    return ret;
}



void CutterCore::seek(QString addr)
{
    if (addr.length() > 0)
        seek(this->math(addr.toUtf8().constData()));
}

void CutterCore::seek(ut64 offset)
{
    CORE_LOCK();
    r_core_seek(this->core_, offset, true);
    emit seekChanged(core_->offset);
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

    //r_core_file_close (this->core, cf);

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

int CutterCore::fcnCyclomaticComplexity(ut64 addr)
{
    CORE_LOCK();
    RAnalFunction *fcn = r_anal_get_fcn_at(core_->anal, addr, addr);
    if (fcn)
        return r_anal_fcn_cc(fcn);
    return 0;
}

int CutterCore::fcnBasicBlockCount(ut64 addr)
{
    CORE_LOCK();
    //RAnalFunction *fcn = r_anal_get_fcn_at (core_->anal, addr, addr);
    RAnalFunction *fcn = r_anal_get_fcn_in(core_->anal, addr, 0);
    if (fcn)
    {
        return r_list_length(fcn->bbs);
    }
    return 0;
}

int CutterCore::fcnEndBbs(RVA addr)
{
    CORE_LOCK();
    RAnalFunction *fcn = r_anal_get_fcn_in(core_->anal, addr, 0);
    if (fcn)
    {
        QString tmp = this->cmd("afi @ " + QString::number(addr) + " ~end-bbs").split("\n")[0];
        if (tmp.contains(":"))
        {
            QString endbbs = tmp.split(": ")[1];
            return endbbs.toInt();
        }
    }

    return 0;
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

void CutterCore::triggerAsmOptionsChanged()
{
    emit asmOptionsChanged();
}

void CutterCore::resetDefaultAsmOptions()
{
    Settings settings;
    setConfig("asm.esil", settings.getAsmESIL());
    setConfig("asm.pseudo", settings.getAsmPseudo());
    setConfig("asm.offset", settings.getAsmOffset());
    setConfig("asm.describe", settings.getAsmDescribe());
    setConfig("asm.stackptr", settings.getAsmStackPointer());
    setConfig("asm.bytes", settings.getAsmBytes());
    setConfig("asm.bytespace", settings.getAsmBytespace());
    setConfig("asm.lbytes", settings.getAsmLBytes());
    setConfig("asm.syntax", settings.getAsmSyntax());
    setConfig("asm.ucase", settings.getAsmUppercase());
    setConfig("asm.bbline", settings.getAsmBBLine());
    setConfig("asm.capitalize", settings.getAsmCapitalize());
    setConfig("asm.varsub", settings.getAsmVarsub());
    setConfig("asm.varsub_only", settings.getAsmVarsubOnly());
}

void CutterCore::saveDefaultAsmOptions()
{
    Settings settings;
    settings.setAsmESIL(getConfigb("asm.esil"));
    settings.setAsmPseudo(getConfigb("asm.pseudo"));
    settings.setAsmOffset(getConfigb("asm.offset"));
    settings.setAsmDescribe(getConfigb("asm.describe"));
    settings.setAsmStackPointer(getConfigb("asm.stackptr"));
    settings.setAsmBytes(getConfigb("asm.bytes"));
    settings.setAsmBytespace(getConfigb("asm.bytespace"));
    settings.setAsmLBytes(getConfigb("asm.lbytes"));
    settings.setAsmSyntax(getConfig("asm.syntax"));
    settings.setAsmUppercase(getConfigb("asm.ucase"));
    settings.setAsmBBLine(getConfigb("asm.bbline"));
    settings.setAsmCapitalize(getConfigb("asm.capitalize"));
    settings.setAsmVarsub(getConfigb("asm.varsub"));
    settings.setAsmVarsubOnly(getConfigb("asm.varsub_only"));
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

QString CutterCore::getOffsetJump(QString addr)
{
    QString ret = cmd("ao @" + addr + "~jump[1]");
    return ret;
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
    setConfig("scr.color", false);
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

    // Graph colors and design
    cmd("ec graph.true rgb:88FF88");
    cmd("ec graph.false rgb:FF6666");
    cmd("ec graph.trufae rgb:4183D7");
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

        if (!whole_function && !to && xref.from != addr)
            continue;

        if (to && !xrefObject.contains("to"))
            xref.to = addr;
        else
            xref.to = xrefObject["to"].toVariant().toULongLong();

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

void CutterCore::loadPDB(const QString &file)
{
    cmd("idp " + sanitizeStringForCommand(file));
}
