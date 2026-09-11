#ifndef DYNAMICSESSION_H
#define DYNAMICSESSION_H

#include "CoreSession.h"

class RizinTaskDialog;

/**
 * @brief Extends CoreSession to handle dynamic analysis
 *
 * Contains logic for Debugging, emulation, tracing, etc
 */
class CUTTER_EXPORT DynamicSession : public CoreSession
{
    Q_OBJECT

public:
    // To be removed ===========================
    CUTTER_DEPRECATED("Only for legacy compatibility, will be removed")
    static DynamicSession *instance();
    CUTTER_DEPRECATED("Only for legacy compatibility, should not be used for anything other than "
                      "signal/slot connections")
    RizinWrapper *getWrapper() const;
    // =========================================

    explicit DynamicSession(QObject *parent = nullptr);
    ~DynamicSession() override = default;

    void setCurrentDebugThread(int tid);
    /**
     * @brief Attach to a given pid from a debug session
     */
    void setCurrentDebugProcess(int pid);

    void startDebug();
    void startEmulation();
    /**
     * @brief attach to a remote debugger
     * @param uri remote debugger uri
     * @note attachedRemote(bool) signals the result
     */
    void attachRemote(const QString &uri);
    void attachDebug(int pid);
    void stopDebug();
    void suspendDebug();
    void syncAndSeekProgramCounter();
    void continueDebug();
    void continueBackDebug();
    void continueUntilCall();
    void continueUntilSyscall();
    void continueUntilDebug(ut64 offset);
    void stepDebug();
    void stepOverDebug();
    void stepOutDebug();
    void stepBackDebug();

    void startTraceSession();

    QString getActiveDebugPlugin();
    QStringList getDebugPlugins();
    void setDebugPlugin(const QString &plugin);
    bool isDebugTaskInProgress();

    void stopTraceSession();

    bool isCurrentlyDebugging() const;
    bool isCurrentlyEmulating() const;
    bool isCurrentlyRemoteDebugging() const;
    bool isCurrentlyTracing() const;
    /**
     * @brief Check if we can use output/input redirection with the currently debugged process
     */
    bool isRedirectableDebugee();

signals:
    void switchedThread();
    void switchedProcess();

    void attachedRemote(bool successfully);

    /**
     * emitted when debugTask started or finished running
     */
    void debugTaskStateChanged();

    void toggleDebugView();
    /**
     * @brief update all the widgets that are affected by rebasing in debug mode
     */
    void codeRebased();

private:
    bool currentlyDebugging = false;
    bool currentlyEmulating = false;
    bool currentlyTracing = false;
    bool currentlyRemoteDebugging = false;
    int currentlyAttachedToPID = -1;
    QString currentlyOpenFile;

    std::shared_ptr<RizinTask> debugTask;
    RizinTaskDialog *debugTaskDialog;

    RVA offsetPriorDebugging = RVA_INVALID;
};

/**
 * @brief Fix for the legacy Core()->function() syntax, temporarily locks the wrapper and provides
 * access
 *
 * TODO: To be removed
 */
struct LegacyLock
{
    RizinLocked activeLock;

    LegacyLock(CoreSession *session) : activeLock(session->lock()) {}

    RizinLocked &operator->() { return activeLock; }
};

#endif // DYNAMICSESSION_H