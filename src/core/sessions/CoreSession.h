#ifndef CORESESSION_H
#define CORESESSION_H

#include "RizinCpp.h"
#include "RizinWrapper.h"
#include "common/BasicBlockHighlighter.h"
#include "common/BasicInstructionHighlighter.h"
#include "common/Helpers.h"
#include "core/CutterCommon.h"

#include <QDebug>
#include <QDir>
#include <QErrorMessage>
#include <QMap>
#include <QMenu>
#include <QMessageBox>
#include <QMutex>
#include <QObject>
#include <QStringList>

#include <functional>
#include <memory>
#include <rz_heap_glibc.h>
#include <rz_project.h>
#include <type_traits>

class AsyncTaskManager;
class BasicInstructionHighlighter;
class RizinWrapper;
class RizinSignals;
class Decompiler;
class RizinTask;
class RizinCmdTask;
class RizinFunctionTask;

class CoreSession;

#define LOCK(obj, ...)                                                                             \
    [&]() {                                                                                        \
        auto rizin = (obj)->lock();                                                                \
        __VA_ARGS__                                                                                \
    }()

class CUTTER_EXPORT RizinLocked
{
    CoreSession *const session;
    bool locked;

public:
    explicit RizinLocked(CoreSession *session);
    RizinLocked(const RizinLocked &) = delete;
    RizinLocked &operator=(const RizinLocked &) = delete;
    RizinLocked(RizinLocked &&) noexcept;
    ~RizinLocked();
    RizinWrapper *operator->() &;

    // Reduce chance of following misuse of Core()->lock()
    // rizinStruct* foo = rizin_func(Core()->lock()->something, arg);
    operator RzCore *() && = delete;
    RzCore *operator->() && = delete;

    /**
     * @brief Manually releases the lock before current scope ends, nothing happens if the mutex is
     * already unlocked
     */
    void unlock();

    /**
     * @brief Manually acquires the core lock, nothing happens if the mutex is already locked
     */
    void lock();

    /**
     * @brief Returns a pointer to the underlying Rizin core instance
     */
    // TODO: change name
    RzCore *core();
};

class CUTTER_EXPORT CoreSession : public QObject
{
    Q_OBJECT
    friend class RizinTask;
    friend class RizinLocked;

public:
    using QObject::connect;
    using QObject::disconnect;

    explicit CoreSession(QObject *parent = nullptr);
    ~CoreSession() override = default;

    /**
     * @brief Generic connect function for autmatically connecting to either Session or RizinWrapper
     * based on the signal provided
     */
    template<typename C, typename... Args, typename... Rest,
             typename = std::enable_if_t<std::is_base_of_v<CoreSession, C>
                                         || std::is_base_of_v<RizinWrapper, C>>>
    QMetaObject::Connection connect(void (C::*signal)(Args...), Rest &&...rest)
    {
        if constexpr (std::is_base_of_v<RizinWrapper, C>) {
            // Use QueuedConnection to avoid holding the lock for longer than needed
            return QObject::connect(static_cast<C *>(wrapper), signal, std::forward<Rest>(rest)...,
                                    Qt::ConnectionType::QueuedConnection);
        } else {
            return QObject::connect(static_cast<C *>(this), signal, std::forward<Rest>(rest)...);
        }
    }
    /**
     * @brief Generic connect function for autmatically disconnecting from either Session or
     * RizinWrapper signal, based on the type of signal provided
     */
    template<typename C, typename... Args, typename... Rest,
             typename = std::enable_if_t<std::is_base_of_v<CoreSession, C>
                                         || std::is_base_of_v<RizinWrapper, C>>>
    bool disconnect(void (C::*signal)(Args...), Rest &&...rest)
    {
        if constexpr (std::is_base_of_v<RizinWrapper, C>) {
            return QObject::disconnect(static_cast<C *>(wrapper), signal,
                                       std::forward<Rest>(rest)...);
        } else {
            return QObject::disconnect(static_cast<C *>(this), signal, std::forward<Rest>(rest)...);
        }
    }

    /**
     * @brief Executes a lambda function while holding the session lock.
     * Automatically locks before execution and guarantees unlocking when it goes out of scope.
     * Supports returning tuples, single values, or void.
     */
    template<typename Func>
    std::invoke_result_t<Func, RizinLocked &> withLock(Func &&func)
    {
        auto locked = this->lock();
        return std::forward<Func>(func)(locked);
    }

    [[nodiscard]] RizinLocked lock();

    void loadCutterRC();
    void loadDefaultCutterRC();
    QDir getCutterRCDefaultDirectory() const;
    AsyncTaskManager *getAsyncTaskManager() { return asyncTaskManager; }
    /**
     * @brief send a task to Rizin
     * @param fcn the task you want to execute
     * @return execute successful?
     */
    bool asyncTask(std::function<void *(RzCore *)> fcn, std::shared_ptr<RizinTask> &task);
    void functionTask(std::function<void *(RzCore *)> fcn);
    QString cmdTask(const QString &str);

    /**
     * @brief Raise a memory widget showing current offset, prefer last active
     * memory widget.
     */
    void showMemoryWidget();
    /**
     * @brief Seek to @p offset and raise a memory widget showing it.
     * @param offset
     */
    void seekAndShow(ut64 offset);
    /**
     * @brief @see TODO: where is this ??? CutterSession::show(ut64) ????
     * @param thing - addressable expression
     */
    void seekAndShow(const QString &thing);

    void setGraphEmpty(bool empty);
    bool isGraphEmpty() const;

    /**
     * @brief Highlight a specific type in the Types widget
     * @param typeName The name of the type to be shown
     */
    void showTypeInTypesWidget(const QString &typeName);

    void message(const QString &msg, bool debug = false);

    BasicBlockHighlighter *getBBHighlighter();
    BasicInstructionHighlighter *getBIHighlighter();

    void triggerRefreshAll();
    void triggerAsmOptionsChanged();
    void triggerGraphOptionsChanged();
    void triggerDebugOptionsChanged();
    void triggerAnalysisOptionsChanged();
    void triggerSymbolsOptionsChanged();

signals:
    void refreshAll();
    void refreshCodeViews();

    /**
     * emitted when config regarding disassembly display changes
     */
    void asmOptionsChanged();

    /**
     * emitted when config regarding graph display changes
     */
    void graphOptionsChanged();

    /**
     * emitted when config regarding analysis changes
     */
    void analysisOptionsChanged();

    /**
     * emitted when config regarding symbols changes (bin.dbginfo / pdb)
     */
    void symbolsOptionsChanged();

    /**
     * emitted when config regarding debug/esil changes
     */
    void debugOptionsChanged();

    void newMessage(const QString &msg);
    void newDebugMessage(const QString &msg);

    void showMemoryWidgetRequested();
    void showAddressRequested(RVA addr);

    /**
     * @brief emitted when a specific type is requested to be shown in the Types Widget
     */
    void showTypeRequested(const QString &typeName);

protected:
    AsyncTaskManager *asyncTaskManager;
    QErrorMessage msgBox;

    RizinWrapper *wrapper = nullptr;

    // TODO: docs here
    static thread_local CoreSession *currentActiveSession;

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    QMutex mutex;
#else
    QRecursiveMutex mutex;
#endif

private:
    bool emptyGraph = false;
    std::unique_ptr<BasicBlockHighlighter> bbHighlighter;
    BasicInstructionHighlighter biHighlighter;

    QVector<QString> getCutterRCFilePaths() const;

    // TODO: docs here
    int lockDepth = 0;
    // TODO: maybe better name
    void *sleepTask = nullptr;

    // since creating new core objects overrides this callback, hence we need to call this each time
    // a new session is created
    void initConsCallbacks();
    // also note that these functions assume lock is held, to be called from within RizinLocked only
    // TODO: diff name??
    static void *consSleepBegin(void *user);
    static void consSleepEnd(void *user, void *bed);
};

#endif // CORESESSION_H