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

#define CutterRListForeach(list, it, type, x) \
    if (list) for (it = list->head; it && ((x=static_cast<type*>(it->data))); it = it->n)

#define CutterRVectorForeach(vec, it, type) \
	if ((vec) && (vec)->a) \
		for (it = (type *)(vec)->a; (char *)it != (char *)(vec)->a + ((vec)->len * (vec)->elem_size); it = (type *)((char *)it + (vec)->elem_size))

#define APPNAME "Cutter"

#define Core() (CutterCore::instance())

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
#include "common/BasicBlockHighlighter.h"

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
    RVA edges;
    RVA cost;
    RVA calls;
    RVA stackframe;

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
    QString category;
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
    QString section;
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

struct SegmentDescription {
    RVA vaddr;
    RVA paddr;
    RVA size;
    RVA vsize;
    QString name;
    QString perm;
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

struct BinClassBaseClassDescription {
    QString name;
    RVA offset;
};

struct BinClassMethodDescription {
    QString name;
    RVA addr = RVA_INVALID;
    st64 vtableOffset = -1;
};

struct BinClassFieldDescription {
    QString name;
    RVA addr = RVA_INVALID;
};

struct BinClassDescription {
    QString name;
    RVA addr = RVA_INVALID;
    RVA vtableAddr = RVA_INVALID;
    ut64 index = 0;
    QList<BinClassBaseClassDescription> baseClasses;
    QList<BinClassMethodDescription> methods;
    QList<BinClassFieldDescription> fields;
};

struct AnalMethodDescription {
    QString name;
    RVA addr;
    st64 vtableOffset;
};

struct AnalBaseClassDescription {
    QString id;
    RVA offset;
    QString className;
};

struct AnalVTableDescription {
    QString id;
    ut64 offset;
    ut64 addr;
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
    QList<BinClassMethodDescription> methods;
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

struct VariableDescription {
    enum class RefType { SP, BP, Reg };
    RefType refType;
    QString name;
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
Q_DECLARE_METATYPE(RIOPluginDescription)
Q_DECLARE_METATYPE(RCorePluginDescription)
Q_DECLARE_METATYPE(RAsmPluginDescription)
Q_DECLARE_METATYPE(BinClassMethodDescription)
Q_DECLARE_METATYPE(BinClassFieldDescription)
Q_DECLARE_METATYPE(BinClassDescription)
Q_DECLARE_METATYPE(const BinClassDescription *)
Q_DECLARE_METATYPE(const BinClassMethodDescription *)
Q_DECLARE_METATYPE(const BinClassFieldDescription *)
Q_DECLARE_METATYPE(AnalBaseClassDescription)
Q_DECLARE_METATYPE(AnalMethodDescription)
Q_DECLARE_METATYPE(AnalVTableDescription)
Q_DECLARE_METATYPE(ResourcesDescription)
Q_DECLARE_METATYPE(VTableDescription)
Q_DECLARE_METATYPE(TypeDescription)
Q_DECLARE_METATYPE(HeaderDescription)
Q_DECLARE_METATYPE(ZignatureDescription)
Q_DECLARE_METATYPE(SearchDescription)
Q_DECLARE_METATYPE(SectionDescription)
Q_DECLARE_METATYPE(SegmentDescription)
Q_DECLARE_METATYPE(MemoryMapDescription)
Q_DECLARE_METATYPE(BreakpointDescription)
Q_DECLARE_METATYPE(ProcessDescription)
Q_DECLARE_METATYPE(RegisterRefDescription)
Q_DECLARE_METATYPE(VariableDescription)

class CutterCore: public QObject
{
    Q_OBJECT

public:
    explicit CutterCore(QObject *parent = nullptr);
    ~CutterCore();
    static CutterCore *instance();

    void initialize();

    AsyncTaskManager *getAsyncTaskManager() { return asyncTaskManager; }

    RVA getOffset() const                   { return core_->offset; }

    /* Core functions (commands) */
    static QString sanitizeStringForCommand(QString s);
    QString cmd(const char *str);
    QString cmd(const QString &str) { return cmd(str.toUtf8().constData()); }
    QString cmdRaw(const QString &str);
    QJsonDocument cmdj(const char *str);
    QJsonDocument cmdj(const QString &str) { return cmdj(str.toUtf8().constData()); }
    QStringList cmdList(const char *str) { return cmd(str).split('\n', QString::SkipEmptyParts); }
    QStringList cmdList(const QString &str) { return cmdList(str.toUtf8().constData()); }
    QString cmdTask(const QString &str);
    QJsonDocument cmdjTask(const QString &str);
    void cmdEsil(const char *command);
    void cmdEsil(const QString &command) { cmdEsil(command.toUtf8().constData()); }
    QString getVersionInformation();

    QJsonDocument parseJson(const char *res, const char *cmd = nullptr);
    QJsonDocument parseJson(const char *res, const QString &cmd = QString())
    {
        return parseJson(res, cmd.isNull() ? nullptr : cmd.toLocal8Bit().constData());
    }

    /* Functions methods */
    void renameFunction(const QString &oldName, const QString &newName);
    void delFunction(RVA addr);
    void renameFlag(QString old_name, QString new_name);
    RAnalFunction *functionAt(ut64 addr);
    QString cmdFunctionAt(QString addr);
    QString cmdFunctionAt(RVA addr);
    QString createFunctionAt(RVA addr, QString name);

    /* Flags */
    void delFlag(RVA addr);
    void delFlag(const QString &name);
    void addFlag(RVA offset, QString name, RVA size);
    void triggerFlagsChanged();

    /* Edition functions */
    QString getInstructionBytes(RVA addr);
    QString getInstructionOpcode(RVA addr);
    void editInstruction(RVA addr, const QString &inst);
    void nopInstruction(RVA addr);
    void jmpReverse(RVA addr);
    void editBytes(RVA addr, const QString &inst);
    void editBytesEndian(RVA addr, const QString &bytes);

    /* Code/Data */
    void setToCode(RVA addr);
    void setAsString(RVA addr);
    void setToData(RVA addr, int size, int repeat = 1);
    int sizeofDataMeta(RVA addr);

    /* Comments */
    void setComment(RVA addr, const QString &cmt);
    void delComment(RVA addr);
    void setImmediateBase(const QString &r2BaseName, RVA offset = RVA_INVALID);
    void setCurrentBits(int bits, RVA offset = RVA_INVALID);

    /* Classes */
    QList<QString> getAllAnalClasses(bool sorted);
    QList<AnalMethodDescription> getAnalClassMethods(const QString &cls);
    QList<AnalBaseClassDescription> getAnalClassBaseClasses(const QString &cls);
    QList<AnalVTableDescription> getAnalClassVTables(const QString &cls);
    void createNewClass(const QString &cls);
    void renameClass(const QString &oldName, const QString &newName);
    void deleteClass(const QString &cls);
    bool getAnalMethod(const QString &cls, const QString &meth, AnalMethodDescription *desc);
    void renameAnalMethod(const QString &className, const QString &oldMethodName, const QString &newMethodName);
    void setAnalMethod(const QString &cls, const AnalMethodDescription &meth);

    /* File related methods */
    bool loadFile(QString path, ut64 baddr = 0LL, ut64 mapaddr = 0LL, int perms = R_PERM_R,
                  int va = 0, bool loadbin = false, const QString &forceBinPlugin = QString());
    bool tryFile(QString path, bool rw);
    bool openFile(QString path, RVA mapaddr);
    void loadScript(const QString &scriptname);
    QJsonArray getOpenedFiles();

    /* Seek functions */
    void seek(QString thing);
    void seek(ut64 offset);
    void seekPrev();
    void seekNext();
    void updateSeek();
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
    ut64 num(const QString &expr);
    QString itoa(ut64 num, int rdx = 16);

    /* Config functions */
    void setConfig(const char *k, const QString &v);
    void setConfig(const QString &k, const QString &v) { setConfig(k.toUtf8().constData(), v); }
    void setConfig(const char *k, int v);
    void setConfig(const QString &k, int v) { setConfig(k.toUtf8().constData(), v); }
    void setConfig(const char *k, bool v);
    void setConfig(const QString &k, bool v) { setConfig(k.toUtf8().constData(), v); }
    void setConfig(const char *k, const QVariant &v);
    void setConfig(const QString &k, const QVariant &v) { setConfig(k.toUtf8().constData(), v); }
    int getConfigi(const char *k);
    int getConfigi(const QString &k) { return getConfigi(k.toUtf8().constData()); }
    ut64 getConfigut64(const char *k);
    ut64 getConfigut64(const QString &k) { return getConfigut64(k.toUtf8().constData()); }
    bool getConfigb(const char *k);
    bool getConfigb(const QString &k) { return getConfigb(k.toUtf8().constData()); }
    QString getConfig(const char *k);
    QString getConfig(const QString &k) { return getConfig(k.toUtf8().constData()); }
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

    /* Debug */
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
    bool isBreakpoint(const QList<RVA> &breakpoints, RVA addr);
    QList<RVA> getBreakpointsAddresses();
    QString getActiveDebugPlugin();
    QStringList getDebugPlugins();
    void setDebugPlugin(QString plugin);
    bool currentlyDebugging = false;
    bool currentlyEmulating = false;
    int currentlyAttachedToPID = -1;
    QString currentlyOpenFile;

    /* Pseudocode */
    QString getDecompiledCodePDC(RVA addr);
    bool getR2DecAvailable();
    QString getDecompiledCodeR2Dec(RVA addr);

    RVA getOffsetJump(RVA addr);
    QJsonDocument getFileInfo();
    QJsonDocument getSignatureInfo();
    QJsonDocument getFileVersionInfo();
    QStringList getStats();
    void setGraphEmpty(bool empty);
    bool isGraphEmpty();

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
    QList<RBinPluginDescription> getRBinPluginDescriptions(const QString &type = QString());
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
    QList<FlagDescription> getAllFlags(QString flagspace = QString());
    QList<SectionDescription> getAllSections();
    QList<SegmentDescription> getAllSegments();
    QList<EntrypointDescription> getAllEntrypoint();
    QList<BinClassDescription> getAllClassesFromBin();
    QList<BinClassDescription> getAllClassesFromFlags();
    QList<ResourcesDescription> getAllResources();
    QList<VTableDescription> getAllVTables();

    /*!
     * \return all loaded types
     */
    QList<TypeDescription> getAllTypes();

    /*!
     * \return all loaded primitive types
     */
    QList<TypeDescription> getAllPrimitiveTypes();

    /*!
     * \return all loaded unions
     */
    QList<TypeDescription> getAllUnions();

    /*!
     * \return all loaded structs
     */
    QList<TypeDescription> getAllStructs();

    /*!
     * \return all loaded enums
     */
    QList<TypeDescription> getAllEnums();

    /*!
     * \return all loaded typedefs
     */
    QList<TypeDescription> getAllTypedefs();

    /*!
     * \brief Adds new types
     * It first uses the r_parse_c_string() function from radare2 API to parse the
     * supplied C file (in the form of a string). If there were errors, they are displayed.
     * If there were no errors, it uses sdb_query_lines() function from radare2 API
     * to save the parsed types returned by r_parse_c_string()
     * \param str Contains the definition of the data types
     * \return returns an empty QString if there was no error, else returns the error
     */
    QString addTypes(const char *str);
    QString addTypes(const QString &str) { return addTypes(str.toUtf8().constData()); }

    QList<MemoryMapDescription> getMemoryMap();
    QList<SearchDescription> getAllSearch(QString search_for, QString space);
    BlockStatistics getBlockStatistics(unsigned int blocksCount);
    QList<BreakpointDescription> getBreakpoints();
    QList<ProcessDescription> getAllProcesses();
    QList<RegisterRefDescription> getRegisterRefs();
    QJsonObject getRegisterJson();
    QList<VariableDescription> getVariables(RVA at);

    QList<XrefDescription> getXRefs(RVA addr, bool to, bool whole_function,
                                    const QString &filterType = QString::null);

    QList<StringDescription> parseStringsJson(const QJsonDocument &doc);
    QList<FunctionDescription> parseFunctionsJson(const QJsonDocument &doc);

    void handleREvent(int type, void *data);

    /* Signals related */
    void triggerVarsChanged();
    void triggerFunctionRenamed(const QString &prevName, const QString &newName);
    void triggerRefreshAll();
    void triggerAsmOptionsChanged();
    void triggerGraphOptionsChanged();

    void message(const QString &msg, bool debug = false);

    QStringList getSectionList();

    RCoreLocked core() const;

    static QString ansiEscapeToHtml(const QString &text);
    BasicBlockHighlighter *getBBHighlighter();

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

    void classNew(const QString &cls);
    void classDeleted(const QString &cls);
    void classRenamed(const QString &oldName, const QString &newName);
    void classAttrsChanged(const QString &cls);

    void projectSaved(bool successfully, const QString &name);

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

    void newMessage(const QString &msg);
    void newDebugMessage(const QString &msg);

private:
    MemoryWidgetType memoryWidgetPriority;

    QString notes;
    RCore *core_ = nullptr;
    AsyncTaskManager *asyncTaskManager;
    RVA offsetPriorDebugging = RVA_INVALID;
    QErrorMessage msgBox;

    bool emptyGraph = false;
    BasicBlockHighlighter *bbHighlighter;

};

#endif // CUTTER_H
