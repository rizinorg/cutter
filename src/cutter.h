#ifndef CUTTER_H
#define CUTTER_H

#include <QMap>
#include <QDebug>
#include <QObject>
#include <QStringList>
#include <QMessageBox>
#include <QJsonDocument>


// Workaround for compile errors on Windows
#ifdef _WIN32
#include <r2hacks.h>
#endif

#include "r_core.h"

// Workaround for compile errors on Windows
#ifdef _WIN32
#undef min
#undef max
#endif //_WIN32

#define HAVE_LATEST_LIBR2 false

#define CutterRListForeach(list, it, type, x) \
    if (list) for (it = list->head; it && ((x=(type*)it->data)); it = it->n)

#define __alert(x) QMessageBox::question (this, "Alert", QString(x), QMessageBox::Ok)
#define __question(x) (QMessageBox::Yes==QMessageBox::question (this, "Alert", QString(x), QMessageBox::Yes| QMessageBox::No))

#define APPNAME "Cutter"

typedef ut64 RVA;
#define RVA_INVALID UT64_MAX

class RCoreLocked
{
    RCore *core;

public:
    explicit RCoreLocked(RCore *core);
    RCoreLocked(const RCoreLocked &) = delete;
    RCoreLocked &operator=(const RCoreLocked &) = delete;
    RCoreLocked(RCoreLocked &&);
    ~RCoreLocked();
    operator RCore *() const;
    RCore *operator->() const;
    RVA seek(RVA offset);
    RVA getSeek();
};

inline QString RAddressString(RVA addr)
{
    return QString::asprintf("%#010llx", addr);
}

inline QString RSizeString(RVA size)
{
    return QString::asprintf("%lld", size);
}

struct FunctionDescription
{
    RVA offset;
    RVA size;
    QString name;

    bool contains(RVA addr) const     { return addr >= offset && addr < offset + size; }
};

struct ImportDescription
{
    RVA plt;
    int ordinal;
    QString bind;
    QString type;
    QString name;
};

struct ExportDescription
{
    RVA vaddr;
    RVA paddr;
    RVA size;
    QString type;
    QString name;
    QString flag_name;
};

struct SymbolDescription
{
    RVA vaddr;
    QString bind;
    QString type;
    QString name;
};

struct CommentDescription
{
    RVA offset;
    QString name;
};

struct RelocDescription
{
    RVA vaddr;
    RVA paddr;
    QString type;
    QString name;
};

struct StringDescription
{
    RVA vaddr;
    QString string;
};

struct FlagspaceDescription
{
    QString name;
};

struct FlagDescription
{
    RVA offset;
    RVA size;
    QString name;
};

struct SectionDescription
{
    RVA vaddr;
    RVA paddr;
    RVA size;
    RVA vsize;
    QString name;
    QString flags;
};

struct EntrypointDescription
{
    RVA vaddr;
    RVA paddr;
    RVA baddr;
    RVA laddr;
    RVA haddr;
    QString type;
};

struct XrefDescription
{
    RVA from;
    RVA to;
    QString type;
};

struct RBinPluginDescription
{
    QString name;
    QString description;
    QString license;
    QString type;
};

Q_DECLARE_METATYPE(FunctionDescription)
Q_DECLARE_METATYPE(ImportDescription)
Q_DECLARE_METATYPE(ExportDescription)
Q_DECLARE_METATYPE(SymbolDescription)
Q_DECLARE_METATYPE(CommentDescription)
Q_DECLARE_METATYPE(RelocDescription)
Q_DECLARE_METATYPE(StringDescription)
Q_DECLARE_METATYPE(FlagspaceDescription)
Q_DECLARE_METATYPE(FlagDescription)
Q_DECLARE_METATYPE(XrefDescription)
Q_DECLARE_METATYPE(EntrypointDescription)
Q_DECLARE_METATYPE(RBinPluginDescription)

class CutterCore: public QObject
{
    Q_OBJECT
    friend class ccClass;

public:
    QString projectPath;

    explicit CutterCore(QObject *parent = 0);
    ~CutterCore();
    static CutterCore* getInstance();

    /* Getters */
    RVA getOffset() const { return core_->offset; }
    int getCycloComplex(ut64 addr);
    int getFcnSize(ut64 addr);
    int fcnCyclomaticComplexity(ut64 addr);
    int fcnBasicBlockCount(ut64 addr);
    int fcnEndBbs(RVA addr);
    static QString sanitizeStringForCommand(QString s);
    QString cmd(const QString &str);
    QJsonDocument cmdj(const QString &str);
    QStringList cmdList(const QString &str)     { auto l = cmd(str).split("\n"); l.removeAll(""); return l; }
    void renameFunction(QString prev_name, QString new_name);
    void setComment(RVA addr, QString cmt);
    void delComment(ut64 addr);
    QMap<QString, QList<QList<QString>>> getNestedComments();
    void setOptions(QString key);
    bool loadFile(QString path, uint64_t loadaddr = 0LL, uint64_t mapaddr = 0LL, bool rw = false, int va = 0, int idx = 0, bool loadbin = false, const QString &forceBinPlugin = nullptr);
    bool tryFile(QString path, bool rw);
    void analyze(int level, QList<QString> advanced);
    void seek(QString addr);
    void seek(ut64 offset);
    ut64 math(const QString &expr);
    QString itoa(ut64 num, int rdx = 16);

    /* Config related */
    void setConfig(const QString &k, const QString &v);
    void setConfig(const QString &k, int v);
    void setConfig(const QString &k, bool v);
    void setConfig(const QString &k, const char *v)     { setConfig(k, QString(v)); }
    int getConfigi(const QString &k);
    bool getConfigb(const QString &k);
    QString getConfig(const QString &k);

    QString assemble(const QString &code);
    QString disassemble(const QString &hex);
    QString disassembleSingleInstruction(RVA addr);
    void setDefaultCPU();
    void setCPU(QString arch, QString cpu, int bits, bool temporary = false);
    RAnalFunction *functionAt(ut64 addr);
    QString cmdFunctionAt(QString addr);
    QString cmdFunctionAt(RVA addr);

    /* SDB */
    QList<QString> sdbList(QString path);
    QList<QString> sdbListKeys(QString path);
    QString sdbGet(QString path, QString key);
    bool sdbSet(QString path, QString key, QString val);
    int get_size();
    ulong get_baddr();
    QList<QList<QString>> get_exec_sections();
    QString getOffsetInfo(QString addr);
    QString getOffsetJump(QString addr);
    QString getDecompiledCode(QString addr);
    QString getFileInfo();
    QStringList getStats();
    QString getSimpleGraph(QString function);
    QString binStart;
    QString binEnd;
    void getOpcodes();
    QList<QString> opcodes;
    QList<QString> regs;
    void setSettings();

    void loadPDB(const QString &file);

    QList<RVA> getSeekHistory();

    QStringList getAsmPluginNames();
    QStringList getAnalPluginNames();

    QStringList getProjectNames();

    QList<RBinPluginDescription> getRBinPluginDescriptions(const QString &type = nullptr);

    QList<FunctionDescription> getAllFunctions();
    QList<ImportDescription> getAllImports();
    QList<ExportDescription> getAllExports();
    QList<SymbolDescription> getAllSymbols();
    QList<CommentDescription> getAllComments(const QString &filterType);
    QList<RelocDescription> getAllRelocs();
    QList<StringDescription> getAllStrings();
    QList<FlagspaceDescription> getAllFlagspaces();
    QList<FlagDescription> getAllFlags(QString flagspace = NULL);
    QList<SectionDescription> getAllSections();
    QList<EntrypointDescription> getAllEntrypoint();

    QList<XrefDescription> getXRefs(RVA addr, bool to, bool whole_function, const QString &filterType = QString::null);

    void addFlag(RVA offset, QString name, RVA size);

    void triggerAsmOptionsChanged();

    void resetDefaultAsmOptions();
    void saveDefaultAsmOptions();

    RCoreLocked core() const;

    /* fields */

    Sdb *db;

signals:
    // TODO: create a more sophisticated update-event system
    void functionRenamed(QString prev_name, QString new_name);
    void flagsChanged();
    void commentsChanged();

    /*!
     * emitted when config regarding disassembly display changes
     */
    void asmOptionsChanged();

    /*!
     * \brief seekChanged is emitted each time radare2 seek value is modified
     * \param offset
     */
    void seekChanged(RVA offset);

public slots:

private:
    QString default_arch;
    QString default_cpu;
    int default_bits;

    RCore *core_;
};

class ccClass : public CutterCore
{
};

#endif // CUTTER_H
