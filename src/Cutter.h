#ifndef CUTTER_H
#define CUTTER_H

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
#include <QErrorMessage>

#define HAVE_LATEST_LIBR2 false

#define CutterRListForeach(list, it, type, x) \
    if (list) for (it = list->head; it && ((x=(type*)it->data)); it = it->n)

#define __alert(x) QMessageBox::question (this, "Alert", QString(x), QMessageBox::Ok)
#define __question(x) (QMessageBox::Yes==QMessageBox::question (this, "Alert", QString(x), QMessageBox::Yes| QMessageBox::No))

#define APPNAME "Cutter"
#define CUTTER_VERSION "1.6"

#define Core() (CutterCore::getInstance())

/*!
 * \brief Type to be used for all kinds of addresses/offsets in r2 address space.
 */
typedef ut64 RVA;

/*!
 * \brief Maximum value of RVA. Do NOT use this for specifying invalid values, use RVA_INVALID instead.
 */
#define RVA_MAX UT64_MAX

/*!
 * \brief Value for specifying an invalid RVA.
 */
#define RVA_INVALID RVA_MAX

class AsyncTaskManager;
class CutterCore;
#include "plugins/CutterPlugin.h"

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

inline QString RHexString(RVA size)
{
    return QString::asprintf("%#llx", size);
}

struct FunctionDescription {
    RVA offset;
    RVA size;
    RVA nargs;
    RVA nbbs;
    RVA nlocals;
    RVA cc;
    QString calltype;
    QString name;

    bool contains(RVA addr) const
    {
        return addr >= offset && addr < offset + size;
    }
};

struct ImportDescription {
    RVA plt;
    int ordinal;
    QString bind;
    QString type;
    QString name;
};

struct ExportDescription {
    RVA vaddr;
    RVA paddr;
    RVA size;
    QString type;
    QString name;
    QString flag_name;
};

struct HeaderDescription
{
    RVA vaddr;
    RVA paddr;
    QString value;
    QString name;
};

struct ZignatureDescription
{
    QString name;
    QString bytes;
    RVA cc;
    RVA nbbs;
    RVA edges;
    RVA ebbs;
    RVA offset;
    QStringList refs;
};

struct TypeDescription {
    QString type;
    int size;
    QString format;
};

struct SearchDescription {
    RVA offset;
    int size;
    QString code;
    QString data;
};

struct SymbolDescription {
    RVA vaddr;
    QString bind;
    QString type;
    QString name;
};

struct CommentDescription {
    RVA offset;
    QString name;
};

struct RelocDescription {
    RVA vaddr;
    RVA paddr;
    QString type;
    QString name;
};

struct StringDescription {
    RVA vaddr;
    QString string;
    QString type;
    ut32 length;
    ut32 size;
};

struct FlagspaceDescription {
    QString name;
};

struct FlagDescription {
    RVA offset;
    RVA size;
    QString name;
};

struct SectionDescription {
    RVA vaddr;
    RVA paddr;
    RVA size;
    RVA vsize;
    QString name;
    QString flags;
    QString entropy;
};

struct EntrypointDescription {
    RVA vaddr;
    RVA paddr;
    RVA baddr;
    RVA laddr;
    RVA haddr;
    QString type;
};

struct XrefDescription {
    RVA from;
    QString from_str;
    RVA to;
    QString to_str;
    QString type;
};

struct RBinPluginDescription {
    QString name;
    QString description;
    QString license;
    QString type;
};

struct RIOPluginDescription {
    QString name;
    QString description;
    QString license;
    QString permissions;
};

struct RCorePluginDescription {
    QString name;
    QString description;
};

struct RAsmPluginDescription {
    QString name;
    QString architecture;
    QString author;
    QString version;
    QString cpus;
    QString description;
    QString license;
};

struct DisassemblyLine {
    RVA offset;
    QString text;
};

struct ClassMethodDescription {
    QString name;
    RVA addr;
};

struct ClassFieldDescription {
    QString name;
    RVA addr;
};

struct ClassDescription {
    QString name;
    RVA addr;
    ut64 index;
    QList<ClassMethodDescription> methods;
    QList<ClassFieldDescription> fields;
};

struct ResourcesDescription {
    int name;
    RVA vaddr;
    ut64 index;
    QString type;
    ut64 size;
    QString lang;
};

struct VTableDescription {
    RVA addr;
    QList<ClassMethodDescription> methods;
};

struct BlockDescription {
    RVA addr;
    RVA size;
    int flags;
    int functions;
    int inFunctions;
    int comments;
    int symbols;
    int strings;
    ut8 rwx;
};

struct BlockStatistics {
    RVA from;
    RVA to;
    RVA blocksize;
    QList<BlockDescription> blocks;
};

struct MemoryMapDescription {
    RVA addrStart;
    RVA addrEnd;
    QString name;
    QString fileName;
    QString type;
    QString permission;
};

struct BreakpointDescription {
    RVA addr;
    int size;
    QString permission;
    bool hw;
    bool trace;
    bool enabled;
};

struct ProcessDescription {
    int pid;
    int uid;
    QString status;
    QString path;
};

struct RegisterRefDescription {
    QString reg;
    QString value;
    QString ref;
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
Q_DECLARE_METATYPE(RIOPluginDescription)
Q_DECLARE_METATYPE(RCorePluginDescription)
Q_DECLARE_METATYPE(RAsmPluginDescription)
Q_DECLARE_METATYPE(ClassMethodDescription)
Q_DECLARE_METATYPE(ClassFieldDescription)
Q_DECLARE_METATYPE(ClassDescription)
Q_DECLARE_METATYPE(const ClassDescription *)
Q_DECLARE_METATYPE(const ClassMethodDescription *)
Q_DECLARE_METATYPE(const ClassFieldDescription *)
Q_DECLARE_METATYPE(ResourcesDescription)
Q_DECLARE_METATYPE(VTableDescription)
Q_DECLARE_METATYPE(TypeDescription)
Q_DECLARE_METATYPE(HeaderDescription)
Q_DECLARE_METATYPE(ZignatureDescription)
Q_DECLARE_METATYPE(SearchDescription)
Q_DECLARE_METATYPE(SectionDescription)
Q_DECLARE_METATYPE(MemoryMapDescription)
Q_DECLARE_METATYPE(BreakpointDescription)
Q_DECLARE_METATYPE(ProcessDescription)
Q_DECLARE_METATYPE(RegisterRefDescription)

class CutterCore: public QObject
{
    Q_OBJECT
    friend class ccClass;

public:
    explicit CutterCore(QObject *parent = nullptr);
    ~CutterCore();
    static CutterCore *getInstance();

    AsyncTaskManager *getAsyncTaskManager() { return asyncTaskManager; }

    RVA getOffset() const                   { return core_->offset; }

    /* Core functions (commands) */
    static QString sanitizeStringForCommand(QString s);
    QString cmd(const QString &str);
    QString cmdRaw(const QString &str);
    QJsonDocument cmdj(const QString &str);
    QStringList cmdList(const QString &str)
    {
        auto l = cmd(str).split("\n");
        l.removeAll("");
        return l;
    }
    QString cmdTask(const QString &str);
    QJsonDocument cmdjTask(const QString &str);
    void cmdEsil(QString command);
    QString getVersionInformation();

    QJsonDocument parseJson(const char *res, const QString &cmd = QString());

    /* Functions methods */
    void renameFunction(const QString &oldName, const QString &newName);
    void delFunction(RVA addr);
    void renameFlag(QString old_name, QString new_name);
    RAnalFunction *functionAt(ut64 addr);
    QString cmdFunctionAt(QString addr);
    QString cmdFunctionAt(RVA addr);
    QString createFunctionAt(RVA addr, QString name);
    void markString(RVA addr);

    /* Flags */
    void delFlag(RVA addr);
    void delFlag(const QString &name);
    void addFlag(RVA offset, QString name, RVA size);
    void triggerFlagsChanged();

    /* Edition functions */
    void editInstruction(RVA addr, const QString &inst);
    void nopInstruction(RVA addr);
    void jmpReverse(RVA addr);
    void editBytes(RVA addr, const QString &inst);
    void editBytesEndian(RVA addr, const QString &bytes);

    /* Comments */
    void setComment(RVA addr, const QString &cmt);
    void delComment(RVA addr);
    void setImmediateBase(const QString &r2BaseName, RVA offset = RVA_INVALID);
    void setCurrentBits(int bits, RVA offset = RVA_INVALID);

    /* File related methods */
    bool loadFile(QString path, ut64 baddr = 0LL, ut64 mapaddr = 0LL, int perms = R_IO_READ,
                  int va = 0, bool loadbin = false, const QString &forceBinPlugin = nullptr);
    bool tryFile(QString path, bool rw);
    void openFile(QString path, RVA mapaddr);
    void loadScript(const QString &scriptname);
    QJsonArray getOpenedFiles();

    /* Analysis functions */
    void analyze(int level, QList<QString> advanced);

    /* Seek functions */
    void seek(QString thing);
    void seek(ut64 offset);
    void seekPrev();
    void seekNext();
    RVA getOffset();
    RVA prevOpAddr(RVA startAddr, int count);
    RVA nextOpAddr(RVA startAddr, int count);

    /* Disassembly/Graph/Hexdump/Pseudocode view priority */
    enum class MemoryWidgetType { Disassembly, Graph, Hexdump, Pseudocode };
    MemoryWidgetType getMemoryWidgetPriority() const
    {
        return memoryWidgetPriority;
    }
    void setMemoryWidgetPriority(MemoryWidgetType type)
    {
        memoryWidgetPriority = type;
    }
    void triggerRaisePrioritizedMemoryWidget()
    {
        emit raisePrioritizedMemoryWidget(memoryWidgetPriority);
    }

    /* Math functions */
    ut64 math(const QString &expr);
    QString itoa(ut64 num, int rdx = 16);

    /* Config functions */
    void setConfig(const QString &k, const QString &v);
    void setConfig(const QString &k, int v);
    void setConfig(const QString &k, bool v);
    void setConfig(const QString &k, const char *v) { setConfig(k, QString(v)); }
    void setConfig(const QString &k, const QVariant &v);
    int getConfigi(const QString &k);
    bool getConfigb(const QString &k);
    QString getConfig(const QString &k);
    QList<QString> getColorThemes();

    /* Assembly related methods */
    QString assemble(const QString &code);
    QString disassemble(const QString &hex);
    QString disassembleSingleInstruction(RVA addr);
    QList<DisassemblyLine> disassembleLines(RVA offset, int lines);
    void setCPU(QString arch, QString cpu, int bits);
    void setEndianness(bool big);
    void setBBSize(int size);

    /* SDB */
    QList<QString> sdbList(QString path);
    QList<QString> sdbListKeys(QString path);
    QString sdbGet(QString path, QString key);
    bool sdbSet(QString path, QString key, QString val);
    int get_size();
    ulong get_baddr();
    QList<QList<QString>> get_exec_sections();
    QString getOffsetInfo(QString addr);

    // Debug
    QJsonDocument getRegistersInfo();
    QJsonDocument getRegisterValues();
    QString getRegisterName(QString registerRole);
    RVA getProgramCounterValue();
    void setRegister(QString regName, QString regValue);
    QJsonDocument getStack(int size = 0x100);
    QJsonDocument getBacktrace();
    void startDebug();
    void startEmulation();
    void attachDebug(int pid);
    void stopDebug();
    void continueDebug();
    void continueUntilCall();
    void continueUntilSyscall();
    void continueUntilDebug(QString offset);
    void stepDebug();
    void stepOverDebug();
    void stepOutDebug();
    void toggleBreakpoint(RVA addr);
    void toggleBreakpoint(QString addr);
    void delBreakpoint(RVA addr);
    void delAllBreakpoints();
    void enableBreakpoint(RVA addr);
    void disableBreakpoint(RVA addr);
    QString getActiveDebugPlugin();
    QStringList getDebugPlugins();
    void setDebugPlugin(QString plugin);
    bool currentlyDebugging = false;
    bool currentlyEmulating = false;
    int currentlyAttachedToPID = -1;
    QString currentlyOpenFile;

    RVA getOffsetJump(RVA addr);
    QString getDecompiledCode(RVA addr);
    QString getDecompiledCode(QString addr);
    QJsonDocument getFileInfo();
    QJsonDocument getSignatureInfo();
    QJsonDocument getFileVersionInfo();
    QStringList getStats();
    QString getSimpleGraph(QString function);

    void getOpcodes();
    QList<QString> opcodes;
    QList<QString> regs;
    void setSettings();

    void loadPDB(const QString &file);

    QList<RVA> getSeekHistory();

    /* Plugins */
    QStringList getAsmPluginNames();
    QStringList getAnalPluginNames();

    /* Projects */
    QStringList getProjectNames();
    void openProject(const QString &name);
    void saveProject(const QString &name);
    void deleteProject(const QString &name);
    static bool isProjectNameValid(const QString &name);

    /* Widgets */
    QList<RBinPluginDescription> getRBinPluginDescriptions(const QString &type = nullptr);
    QList<RIOPluginDescription> getRIOPluginDescriptions();
    QList<RCorePluginDescription> getRCorePluginDescriptions();
    QList<RAsmPluginDescription> getRAsmPluginDescriptions();
    QList<FunctionDescription> getAllFunctions();
    QList<ImportDescription> getAllImports();
    QList<ExportDescription> getAllExports();
    QList<SymbolDescription> getAllSymbols();
    QList<HeaderDescription> getAllHeaders();
    QList<ZignatureDescription> getAllZignatures();
    QList<CommentDescription> getAllComments(const QString &filterType);
    QList<RelocDescription> getAllRelocs();
    QList<StringDescription> getAllStrings();
    QList<FlagspaceDescription> getAllFlagspaces();
    QList<FlagDescription> getAllFlags(QString flagspace = NULL);
    QList<SectionDescription> getAllSections();
    QList<EntrypointDescription> getAllEntrypoint();
    QList<ClassDescription> getAllClassesFromBin();
    QList<ClassDescription> getAllClassesFromFlags();
    QList<ResourcesDescription> getAllResources();
    QList<VTableDescription> getAllVTables();
    QList<TypeDescription> getAllTypes();
    QList<MemoryMapDescription> getMemoryMap();
    QList<SearchDescription> getAllSearch(QString search_for, QString space);
    BlockStatistics getBlockStatistics(unsigned int blocksCount);
    QList<BreakpointDescription> getBreakpoints();
    QList<ProcessDescription> getAllProcesses();
    QList<RegisterRefDescription> getRegisterRefs();
    QJsonObject getRegisterJson();

    QList<XrefDescription> getXRefs(RVA addr, bool to, bool whole_function,
                                    const QString &filterType = QString::null);

    QList<StringDescription> parseStringsJson(const QJsonDocument &doc);
    QList<FunctionDescription> parseFunctionsJson(const QJsonDocument &doc);

    /* Signals related */
    void triggerVarsChanged();
    void triggerFunctionRenamed(const QString &prevName, const QString &newName);
    void triggerRefreshAll();
    void triggerAsmOptionsChanged();
    void triggerGraphOptionsChanged();

    void setCutterPlugins(QList<CutterPlugin*> plugins);
    QList<CutterPlugin*> getCutterPlugins();

    RCoreLocked core() const;

signals:
    void refreshAll();

    void functionRenamed(const QString &prev_name, const QString &new_name);
    void varsChanged();
    void functionsChanged();
    void flagsChanged();
    void commentsChanged();
    void registersChanged();
    void instructionChanged(RVA offset);
    void breakpointsChanged();
    void refreshCodeViews();
    void stackChanged();

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
    void changeDefinedView();
    void changeDebugView();

public slots:

private:
    MemoryWidgetType memoryWidgetPriority;

    QString notes;
    RCore *core_;
    AsyncTaskManager *asyncTaskManager;
    RVA offsetPriorDebugging = RVA_INVALID;
    QErrorMessage msgBox;

    QList<CutterPlugin*> plugins;
};

class ccClass : public CutterCore
{
};

#endif // CUTTER_H
