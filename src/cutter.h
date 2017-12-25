#ifndef CUTTER_H
#define CUTTER_H

// Workaround for compile errors on Windows
#ifdef _WIN32
#include <r_addr_interval_msvc.h>
#endif //_WIN32

#include "r_core.h"

// Workaround for compile errors on Windows
#ifdef _WIN32
#undef min
#undef max
#endif //_WIN32

#include <QMap>
#include <QDebug>
#include <QObject>
#include <QStringList>
#include <QMessageBox>
#include <QJsonDocument>

#define HAVE_LATEST_LIBR2 false

#define CutterRListForeach(list, it, type, x) \
    if (list) for (it = list->head; it && ((x=(type*)it->data)); it = it->n)

#define __alert(x) QMessageBox::question (this, "Alert", QString(x), QMessageBox::Ok)
#define __question(x) (QMessageBox::Yes==QMessageBox::question (this, "Alert", QString(x), QMessageBox::Yes| QMessageBox::No))

#define APPNAME "Cutter"
#define CUTTER_VERSION "1.1"

#define Core() (CutterCore::getInstance())

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
    QString from_str;
    RVA to;
    QString to_str;
    QString type;
};

struct RBinPluginDescription
{
    QString name;
    QString description;
    QString license;
    QString type;
};

struct DisassemblyLine
{
    RVA offset;
    QString text;
};

struct ClassMethodDescription
{
    QString name;
    RVA addr;
};

struct ClassFieldDescription
{
    QString name;
    RVA addr;
};

struct ClassDescription
{
    QString name;
    RVA addr;
    ut64 index;
    QList<ClassMethodDescription> methods;
    QList<ClassFieldDescription> fields;
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
Q_DECLARE_METATYPE(ClassMethodDescription)
Q_DECLARE_METATYPE(ClassFieldDescription)
Q_DECLARE_METATYPE(ClassDescription)
Q_DECLARE_METATYPE(const ClassDescription *)
Q_DECLARE_METATYPE(const ClassMethodDescription *)
Q_DECLARE_METATYPE(const ClassFieldDescription *)

class CutterCore: public QObject
{
    Q_OBJECT
    friend class ccClass;

public:
    explicit CutterCore(QObject *parent = 0);
    ~CutterCore();
    static CutterCore *getInstance();

    /* Getters */
    RVA getOffset() const { return core_->offset; }

    static QString sanitizeStringForCommand(QString s);
    QString cmd(const QString &str);
    QString cmdRaw(const QString &str);
    QJsonDocument cmdj(const QString &str);
    QStringList cmdList(const QString &str)     { auto l = cmd(str).split("\n"); l.removeAll(""); return l; }

    QList<DisassemblyLine> disassembleLines(RVA offset, int lines);

    void renameFunction(const QString &oldName, const QString &newName);
    void delFunction(RVA addr);
    void renameFlag(QString old_name, QString new_name);
    void delFlag(RVA addr);

    void setComment(RVA addr, const QString &cmt);
    void delComment(RVA addr);

    void setImmediateBase(const QString &r2BaseName, RVA offset = RVA_INVALID);

    void setOptions(QString key);
    bool loadFile(QString path, uint64_t loadaddr = 0LL, uint64_t mapaddr = 0LL, bool rw = false, int va = 0, int idx = 0, bool loadbin = false, const QString &forceBinPlugin = nullptr);
    bool tryFile(QString path, bool rw);
    void analyze(int level, QList<QString> advanced);

    // Seek functions
    void seek(QString addr);
    void seek(ut64 offset);
    void seekPrev();
    void seekNext();
    RVA getOffset();

    RVA prevOpAddr(RVA startAddr, int count);
    RVA nextOpAddr(RVA startAddr, int count);

    // Disassembly/Graph/Hexdump/Pseudocode view priority
    enum class MemoryWidgetType { Disassembly, Graph, Hexdump, Pseudocode };
    MemoryWidgetType getMemoryWidgetPriority() const            { return memoryWidgetPriority; }
    void setMemoryWidgetPriority(MemoryWidgetType type)         { memoryWidgetPriority = type; }
    void triggerRaisePrioritizedMemoryWidget()                  { emit raisePrioritizedMemoryWidget(memoryWidgetPriority); }

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

    QString createFunctionAt(RVA addr, QString name);
    void markString(RVA addr);

    /* SDB */
    QList<QString> sdbList(QString path);
    QList<QString> sdbListKeys(QString path);
    QString sdbGet(QString path, QString key);
    bool sdbSet(QString path, QString key, QString val);
    int get_size();
    ulong get_baddr();
    QList<QList<QString>> get_exec_sections();
    QString getOffsetInfo(QString addr);
    RVA getOffsetJump(RVA addr);
    QString getDecompiledCode(RVA addr);
    QString getDecompiledCode(QString addr);
    QString getFileInfo();
    QStringList getStats();
    QString getSimpleGraph(QString function);

    void getOpcodes();
    QList<QString> opcodes;
    QList<QString> regs;
    void setSettings();

    void loadPDB(const QString &file);

    QList<RVA> getSeekHistory();

    QStringList getAsmPluginNames();
    QStringList getAnalPluginNames();

    QStringList getProjectNames();
    void openProject(const QString &name);
    void saveProject(const QString &name);

    static bool isProjectNameValid(const QString &name);

    const QString &getNotes() const                { return notes; }
    void setNotes(const QString &notes);

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
    QList<ClassDescription> getAllClasses();

    QList<XrefDescription> getXRefs(RVA addr, bool to, bool whole_function, const QString &filterType = QString::null);

    void addFlag(RVA offset, QString name, RVA size);
    void triggerFlagsChanged();

    void triggerVarsChanged();
    void triggerFunctionRenamed(const QString &prevName, const QString &newName);

    void triggerRefreshAll();

    void triggerAsmOptionsChanged();
    void triggerGraphOptionsChanged();

    void resetDefaultAsmOptions();
    void saveDefaultAsmOptions();

    void loadScript(const QString &scriptname);
    QString getVersionInformation();

    RCoreLocked core() const;

    /* fields */

    Sdb *db;

signals:
    void refreshAll();

    void functionRenamed(const QString &prev_name, const QString &new_name);
    void varsChanged();
    void functionsChanged();
    void flagsChanged();
    void commentsChanged();
    void instructionChanged(RVA offset);

    void notesChanged(const QString &notes);
    void projectSaved(const QString &name);

    /*!
     * emitted when config regarding disassembly display changes
     */
    void asmOptionsChanged();

    /*!
     * emitted when config regarding graph display changes
     */
    void graphOptionsChanged();

    /*!
     * \brief seekChanged is emitted each time radare2 seek value is modified
     * \param offset
     */
    void seekChanged(RVA offset);

    void raisePrioritizedMemoryWidget(CutterCore::MemoryWidgetType type);

public slots:

private:
    QString default_arch;
    QString default_cpu;
    int default_bits;

    MemoryWidgetType memoryWidgetPriority;

    QString notes;

    RCore *core_;
};

class ccClass : public CutterCore
{
};

#endif // CUTTER_H
