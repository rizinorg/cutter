#include "qrcore.h"
#include "sdb.h"

#include <QJsonArray>
#include <QJsonObject>

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

RCoreLocked QRCore::core() const
{
    return RCoreLocked(this->core_);
}

#define CORE_LOCK() RCoreLocked core_lock__(this->core_)

QRCore::QRCore(QObject *parent) :
    QObject(parent)
{
    r_cons_new();  // initialize console
    this->projectPath = "";
    this->core_ = r_core_new();
    r_core_loadlibs(this->core_, R_CORE_LOADLIBS_ALL, NULL);
    // IMPLICIT r_bin_iobind (core_->bin, core_->io);

    // Otherwise r2 may ask the user for input and Iaito would freeze
    config("scr.interactive", "false");

    // Used by the HTML5 graph
    config("http.cors", "true");
    config("http.sandbox", "false");
    //config("http.port", "14170");

    // Temporary fixes
    //config("http.root","/usr/local/share/radare2/last/www");
    //config("http.root","/usr/local/radare2/osx/share/radare2/1.1.0-git/www");

    this->db = sdb_new(NULL, NULL, 0);  // WTF NOES
}

QList<QString> QRCore::getFunctionXrefs(ut64 addr)
{
    CORE_LOCK();
    QList<QString> ret = QList<QString>();
    RList *list = r_anal_xrefs_get(core_->anal, addr);
    RAnalRef *ref;
    RListIter *it;
    QRListForeach(list, it, RAnalRef, ref)
    {
        ret << QString("%1,0x%2,0x%3").arg(
                QString(ref->type),
                QString::number(ref->addr, 16),
                QString::number(ref->at, 16));
    }
    return ret;
}

QList<QString> QRCore::getFunctionRefs(ut64 addr, char type)
{
    CORE_LOCK();
    QList<QString> ret = QList<QString>();
    //RAnalFunction *fcn = r_anal_get_fcn_at(core_->anal, addr, addr);
    RAnalFunction *fcn = r_anal_get_fcn_in(core_->anal, addr, 0);
    if (!fcn)
    {
        eprintf("qcore->getFunctionRefs: No function found\n");
        return ret;
    }
    //eprintf(fcn->name);
    RAnalRef *ref;
    RListIter *it;
    QRListForeach(fcn->refs, it, RAnalRef, ref)
    {
        if (type == ref->type || type == 0)
            ret << QString("%1,0x%2,0x%3").arg(
                    QString(ref->type),
                    QString::number(ref->addr, 16),
                    QString::number(ref->at, 16));
    }
    return ret;
}

int QRCore::getCycloComplex(ut64 addr)
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

int QRCore::getFcnSize(ut64 addr)
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

QList<QString> QRCore::sdbList(QString path)
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

QList<QString> QRCore::sdbListKeys(QString path)
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

QString QRCore::sdbGet(QString path, QString key)
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

bool QRCore::sdbSet(QString path, QString key, QString val)
{
    CORE_LOCK();
    Sdb *db = sdb_ns_path(core_->sdb, path.toUtf8().constData(), 1);
    if (!db) return false;
    return sdb_set(db, key.toUtf8().constData(), val.toUtf8().constData(), 0);
}

QRCore::~QRCore()
{
    r_core_free(this->core_);
    r_cons_free();
}

QString QRCore::cmd(const QString &str)
{
    CORE_LOCK();
    QByteArray cmd = str.toUtf8();
    //r_cons_flush();
    char *res = r_core_cmd_str(this->core_, cmd.constData());
    QString o = QString(res ? res : "");
    //r_mem_free was added in https://github.com/radare/radare2/commit/cd28744049492dc8ac25a1f2b3ba0e42f0e9ce93
    r_mem_free(res);
    return o;
}

QJsonDocument QRCore::cmdj(const QString &str)
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

bool QRCore::loadFile(QString path, uint64_t loadaddr, uint64_t mapaddr, bool rw, int va, int bits, int idx, bool loadbin)
{
    QNOTUSED(loadaddr);
    QNOTUSED(idx);

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
        if (bits != 0)
        {
            r_config_set_i(core_->config, "asm.bits", bits);
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
    r_core_hash_load(core_, path.toUtf8().constData());
    fflush(stdout);
    return true;
}

void QRCore::analyze(int level)
{
    CORE_LOCK();
    /*
     * Levels
     * Nivel 1: afr @ entry0 y main (afr@entry0;afr@main)
     * Nivel 2: aa
     * Nivel 3: aaa
     * Nivel 4: aaaa
     */

    if (level == 1)
    {
        r_core_cmd0(core_, "afr@entry0;afr@main");
    }
    else if (level == 2)
    {
        r_core_cmd0(core_, "aa");
    }
    else if (level == 3)
    {
        r_core_cmd0(core_, "aaa");
    }
    else if (level == 4)
    {
        r_core_cmd0(core_, "aaaa");
    }
}

void QRCore::renameFunction(QString prev_name, QString new_name)
{
    cmd("afn " + new_name + " " + prev_name);
    emit functionRenamed(prev_name, new_name);
}

void QRCore::setComment(RVA addr, QString cmt)
{
    //r_meta_add (core->anal, 'C', addr, 1, cmt.toUtf8());
    cmd("CC " + cmt + " @ " + QString::number(addr));
}

void QRCore::setComment(QString addr, QString cmt)
{
    //r_meta_add (core->anal, 'C', addr, 1, cmt.toUtf8());
    cmd("CC " + cmt + " @ " + addr);
}

void QRCore::delComment(ut64 addr)
{
    CORE_LOCK();
    r_meta_del(core_->anal, 'C', addr, 1, NULL);
    //cmd (QString("CC-@")+addr);
}

QMap<QString, QList<QList<QString>>> QRCore::getNestedComments()
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

void QRCore::seek(QString addr)
{
    if (addr.length() > 0)
        seek(this->math(addr.toUtf8().constData()));
}



void QRCore::seek(ut64 offset)
{
    CORE_LOCK();
    r_core_seek(this->core_, offset, true);
}

bool QRCore::tryFile(QString path, bool rw)
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

    bool is_writable = cf->desc->flags & R_IO_WRITE;
    // if rbin works, tell entry0, and symbols (main, constructor, ..)

    //r_core_file_close (this->core, cf);

    sdb_bool_set(DB, "try.is_writable", is_writable, 0);
    sdb_set(DB, "try.filetype", "elf.i386", 0);
    sdb_set(DB, "try.filename", path.toUtf8().constData(), 0);
    return true;
}

QList<QString> QRCore::getList(const QString &type, const QString &subtype)
{
    CORE_LOCK();
    RListIter *it;
    QList<QString> ret = QList<QString>();

    if (type == "bin")
    {
        if (subtype == "sections")
        {
            QString text = cmd("S*~^S");
            for (QString i : text.split("\n"))
            {
                ret << i.mid(2).replace(" ", ",");
            }
        }
        else if (subtype == "types")
        {
            ret << "raw";
            auto ft = sdb_const_get(DB, "try.filetype", 0);
            if (ft && *ft)
                ret << ft;
        }
        else if (subtype == "entrypoints")
        {
            if (math("entry0") != 0)
                ret << "entry0";
        }
    }
    else if (type == "asm")
    {
        if (subtype == "plugins")
        {
            RAsmPlugin *ap;
            QRListForeach(core_->assembler->plugins, it, RAsmPlugin, ap)
            {
                ret << ap->name;
            }
        }
        else if (subtype == "cpus")
        {
            QString funcs = cmd("e asm.cpu=?");
            QStringList lines = funcs.split("\n");
            for (auto cpu : lines)
            {
                ret << cpu;
            }
        }
    }
    else if (type == "anal")
    {
        if (subtype == "plugins")
        {
            RAnalPlugin *ap;
            QRListForeach(core_->anal->plugins, it, RAnalPlugin, ap)
            {
                ret << ap->name;
            }
        }
    }
    else if (type == "flagspaces")
    {
        QStringList lines = cmd("fs*").split("\n");
        for (auto i : lines)
        {
            QStringList a = i.replace("*", "").split(" ");
            if (a.length() > 1)
                ret << a[1];
        }
    }
    else if (type == "flags")
    {
        if (subtype != NULL && subtype != "")
            cmd("fs " + subtype);
        else cmd("fs *");
        QString flags = cmd("f*");
        QStringList lines = flags.split("\n");
        for (auto i : lines)
        {
            // TODO: is 0 in a string even possible?
            if (i[0] != QChar(0) && i[1] == QChar('s')) continue; // skip 'fs ..'
            ret << i.mid(2).replace(" ", ",");
        }
    }
    return ret;
}

ut64 QRCore::math(const QString &expr)
{
    CORE_LOCK();
    return r_num_math(this->core_ ? this->core_->num : NULL, expr.toUtf8().constData());
}

int QRCore::fcnCyclomaticComplexity(ut64 addr)
{
    CORE_LOCK();
    RAnalFunction *fcn = r_anal_get_fcn_at(core_->anal, addr, addr);
    if (fcn)
        return r_anal_fcn_cc(fcn);
    return 0;
}

int QRCore::fcnBasicBlockCount(ut64 addr)
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

int QRCore::fcnEndBbs(QString addr)
{
    CORE_LOCK();
    bool ok;
    int offset = addr.toLong(&ok, 16);
    RAnalFunction *fcn = r_anal_get_fcn_in(core_->anal, offset, 0);
    if (fcn)
    {
        QString tmp = this->cmd("afi @ " + addr + " ~end-bbs").split("\n")[0];
        if (tmp.contains(":"))
        {
            QString endbbs = tmp.split(": ")[1];
            return endbbs.toInt();
        }
    }

    return 0;
}

QString QRCore::itoa(ut64 num, int rdx)
{
    return QString::number(num, rdx);
}

QString QRCore::config(const QString &k, const QString &v)
{
    CORE_LOCK();
    QByteArray key = k.toUtf8();
    if (v != NULL)
    {
        r_config_set(core_->config, key.constData(), v.toUtf8().constData());
        return NULL;
    }
    return QString(r_config_get(core_->config, key.constData()));
}

int QRCore::config(const QString &k, int v)
{
    CORE_LOCK();
    QByteArray key = k.toUtf8();
    if (v != -1)
    {
        r_config_set_i(core_->config, key.constData(), v);
        return 0;
    }
    return r_config_get_i(core_->config, key.constData());
}

void QRCore::setOptions(QString key)
{
    QNOTUSED(key);

    // va
    // lowercase
    // show bytes
    // att syntax
    // asm plugin
    // cpu type
    // anal plugin
}

void QRCore::setCPU(QString arch, QString cpu, int bits, bool temporary)
{
    config("asm.arch", arch);
    config("asm.cpu", cpu);
    config("asm.bits", bits);
    if (!temporary)
    {
        default_arch = arch;
        default_cpu = cpu;
        default_bits = bits;
    }
}

void QRCore::setDefaultCPU()
{
    config("asm.arch", default_arch);
    config("asm.cpu", default_cpu);
    config("asm.bits", QString::number(default_bits));
}

QString QRCore::assemble(const QString &code)
{
    CORE_LOCK();
    RAsmCode *ac = r_asm_massemble(core_->assembler, code.toUtf8().constData());
    QString hex(ac != nullptr ? ac->buf_hex : "");
    r_asm_code_free(ac);
    return hex;
}

QString QRCore::disassemble(const QString &hex)
{
    CORE_LOCK();
    RAsmCode *ac = r_asm_mdisassemble_hexstr(core_->assembler, hex.toUtf8().constData());
    QString code = QString(ac != nullptr ? ac->buf_asm : "");
    r_asm_code_free(ac);
    return code;
}

RAnalFunction *QRCore::functionAt(ut64 addr)
{
    CORE_LOCK();
    //return r_anal_fcn_find (core_->anal, addr, addr);
    return r_anal_get_fcn_in(core_->anal, addr, 0);
}

QString QRCore::cmdFunctionAt(QString addr)
{
    QString ret;
    //afi~name:1[1] @ 0x08048e44
    //ret = cmd("afi~name[1] @ " + addr);
    ret = cmd(QString("fd @ ") + addr + "~[0]");
    return ret.trimmed();
}

QString QRCore::cmdFunctionAt(RVA addr)
{
    return cmdFunctionAt(QString::number(addr));
}

int QRCore::get_size()
{
    CORE_LOCK();
    RBinObject *obj = r_bin_get_object(core_->bin);
    //return obj->size;
    return obj != nullptr ? obj->obj_size : 0;
}

ulong QRCore::get_baddr()
{
    CORE_LOCK();
    ulong baddr = r_bin_get_baddr(core_->bin);
    return baddr;
}

QList<QList<QString>> QRCore::get_exec_sections()
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

QString QRCore::getOffsetInfo(QString addr)
{
    return cmd("ao @ " + addr);
}

QString QRCore::getOffsetJump(QString addr)
{
    QString ret = cmd("ao @" + addr + "~jump[1]");
    return ret;
}

QString QRCore::getDecompiledCode(QString addr)
{
    return cmd("pdc @ " + addr);
}

QString QRCore::getFileInfo()
{
    QString info = cmd("ij");
    return info;
}

QStringList QRCore::getStats()
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

QString QRCore::getSimpleGraph(QString function)
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

void QRCore::getOpcodes()
{
    QString opcodes = cmd("?O");
    this->opcodes = opcodes.split("\n");
    // Remove the last empty element
    this->opcodes.removeLast();
    QString registers = cmd("drp~[1]");
    this->regs = registers.split("\n");
    this->regs.removeLast();
}

void QRCore::setSettings()
{
    config("scr.color", "false");
    config("scr.interactive", "false");
    config("asm.lines", "false");
    // Intredazting...
    //config("asm.linesright", "true");
    //config("asm.lineswidth", "15");
    //config("asm.functions", "false");
    config("hex.pairs", "false");
    config("asm.bytespace", "true");
    config("asm.cmtflgrefs", "false");
    config("asm.cmtright", "true");
    config("asm.cmtcol", "70");
    config("asm.xrefs", "false");
    config("asm.fcnlines", "false");

    config("asm.tabs", "5");
    config("asm.tabsonce", "true");
    config("asm.tabsoff", "5");
    config("asm.nbytes", "10");
    config("asm.midflags", "2");
    //config("asm.bbline", "true");

    config("anal.hasnext", "true");
    config("asm.fcncalls", "false");
    config("asm.calls", "false");
    config("asm.lines.call", "false");
    config("asm.flgoff", "true");
    config("anal.autoname", "true");

    // Highlight current node in graphviz
    config("graph.gv.current", "true");

    // Fucking pancake xD
    config("cfg.fortunes.tts", "false");

    // Experimenting with asm options
    //config("asm.spacy", "true");      // We need to handle blank lines on scroll
    //config("asm.section", "true");    // Breaks the disasm and navigation
    //config("asm.invhex", "true");     // Needs further testing
    //config("asm.flags", "false");     // Add with default true in future

    // Used by the HTML5 graph
    config("http.cors", "true");
    config("http.sandbox", "false");
    //config("http.port", "14170");

    // Temporary fixes
    //config("http.root","/usr/local/share/radare2/last/www");
    //config("http.root","/usr/local/radare2/osx/share/radare2/1.1.0-git/www");
    //config("bin.rawstr", "true");

    // Graph colors and design
    cmd("ec graph.true rgb:88FF88");
    cmd("ec graph.false rgb:FF6666");
    cmd("ec graph.trufae rgb:4183D7");
}





QList<RVA> QRCore::getSeekHistory()
{
    CORE_LOCK();
    QList<RVA> ret;

    QJsonArray jsonArray = cmdj("sj").array();
    foreach (QJsonValue value, jsonArray)
        ret << value.toVariant().toULongLong();

    return ret;
}

QList<FunctionDescription> QRCore::getAllFunctions()
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


QList<ImportDescription> QRCore::getAllImports()
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


QList<SymbolDescription> QRCore::getAllSymbols()
{
    CORE_LOCK();
    RListIter *it;

    QList<SymbolDescription> ret;

    RBinSymbol *bs;
    if (core_ && core_->bin && core_->bin->cur && core_->bin->cur->o)
    {
        QRListForeach(core_->bin->cur->o->symbols, it, RBinSymbol, bs)
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
        QRListForeach(core_->bin->cur->o->entries, it, RBinAddr, entry)
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


QList<CommentDescription> QRCore::getAllComments(const QString &filterType)
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

QList<RelocDescription> QRCore::getAllRelocs()
{
    CORE_LOCK();
    RListIter *it;
    QList<RelocDescription> ret;

    RBinReloc *br;
    if (core_ && core_->bin && core_->bin->cur && core_->bin->cur->o)
    {
        QRListForeach(core_->bin->cur->o->relocs, it, RBinReloc, br)
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

QList<StringDescription> QRCore::getAllStrings()
{
    CORE_LOCK();
    RListIter *it;
    QList<StringDescription> ret;

    RBinString *bs;
    if (core_ && core_->bin && core_->bin->cur && core_->bin->cur->o)
    {
        QRListForeach(core_->bin->cur->o->strings, it, RBinString, bs)
        {
            StringDescription str;
            str.vaddr = bs->vaddr;
            str.string = bs->string;
            ret << str;
        }
    }

    return ret;
}
