#ifndef CUTTER_H
#define CUTTER_H

#include "core/CutterCommon.h"
#include "core/CutterDescriptions.h"

#include <QMap>
#include <QDebug>
#include <QObject>
#include <QStringList>
#include <QMessageBox>
#include <QJsonDocument>
#include <QErrorMessage>

class AsyncTaskManager;
class CutterCore;
#include "plugins/CutterPlugin.h"
#include "common/BasicBlockHighlighter.h"

#define Core() (CutterCore::instance())

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
    QStringList cmdList(const char *str) { return cmd(str).split(QLatin1Char('\n'), QString::SkipEmptyParts); }
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

    QStringList autocomplete(const QString &cmd, RLinePromptType promptType, size_t limit = 4096);

    /* Functions methods */
    void renameFunction(const QString &oldName, const QString &newName);
    void delFunction(RVA addr);
    void renameFlag(QString old_name, QString new_name);
    RAnalFunction *functionAt(ut64 addr);
    QString cmdFunctionAt(QString addr);
    QString cmdFunctionAt(RVA addr);
    QString createFunctionAt(RVA addr, QString name);
    QStringList getDisassemblyPreview(RVA address, int num_of_lines);

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

    /**
     * @brief Changes immediate displacement to structure offset
     * This function makes use of the "ta" command of r2 to apply structure
     * offset to the immediate displacement used in the given instruction
     * \param structureOffset The name of struct which will be applied
     * \param offset The address of the instruction where the struct will be applied
     */
    void applyStructureOffset(const QString &structureOffset, RVA offset = RVA_INVALID);

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

    /* Assembly\Hexdump related methods */
    QByteArray assemble(const QString &code);
    QString disassemble(const QByteArray &data);
    QString disassembleSingleInstruction(RVA addr);
    QList<DisassemblyLine> disassembleLines(RVA offset, int lines);

    static QByteArray hexStringToBytes(const QString &hex);
    static QString bytesToHexString(const QByteArray &bytes);
    enum class HexdumpFormats { Normal, Half, Word, Quad, Signed, Octal };
    QString hexdump(RVA offset, int size, HexdumpFormats format);
    QString getHexdumpPreview(RVA offset, int size);

    void setCPU(QString arch, QString cpu, int bits);
    void setEndianness(bool big);

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

    QByteArray ioRead(RVA addr, int len);

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

    /**
     * @return all loaded types
     */
    QList<TypeDescription> getAllTypes();

    /**
     * @return all loaded primitive types
     */
    QList<TypeDescription> getAllPrimitiveTypes();

    /**
     * @return all loaded unions
     */
    QList<TypeDescription> getAllUnions();

    /**
     * @return all loaded structs
     */
    QList<TypeDescription> getAllStructs();

    /**
     * @return all loaded enums
     */
    QList<TypeDescription> getAllEnums();

    /**
     * @return all loaded typedefs
     */
    QList<TypeDescription> getAllTypedefs();

    /**
     * @brief Adds new types
     * It first uses the r_parse_c_string() function from radare2 API to parse the
     * supplied C file (in the form of a string). If there were errors, they are displayed.
     * If there were no errors, it uses sdb_query_lines() function from radare2 API
     * to save the parsed types returned by r_parse_c_string()
     * \param str Contains the definition of the data types
     * \return returns an empty QString if there was no error, else returns the error
     */
    QString addTypes(const char *str);
    QString addTypes(const QString &str) { return addTypes(str.toUtf8().constData()); }

    /**
     * @brief Checks if the given address is mapped to a region
     * @param addr The address to be checked
     * @return true if addr is mapped, false otherwise
     */
    bool isAddressMapped(RVA addr);

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

    /**
     * emitted when config regarding disassembly display changes
     */
    void asmOptionsChanged();

    /**
     * emitted when config regarding graph display changes
     */
    void graphOptionsChanged();

    /**
     * @brief seekChanged is emitted each time radare2 seek value is modified
     * @param offset
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
