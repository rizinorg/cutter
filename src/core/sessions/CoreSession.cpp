#include "CoreSession.h"

#include "common/AsyncTask.h"
#include "common/BasicInstructionHighlighter.h"
#include "common/RizinTask.h"

#include <QCoreApplication>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QStringList>
#include <QTimer>
#include <QVector>

#include <cassert>
#include <memory>
#include <rz_asm.h>
#include <rz_cmd.h>
#include <rz_socket.h>
#include <sdb.h>

thread_local CoreSession *CoreSession::currentActiveSession = nullptr;

CoreSession::CoreSession(QObject *parent)
    : QObject(parent)
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
      ,
      mutex(QMutex::Recursive)
#endif
{
    wrapper = new RizinWrapper(this);

    auto returnFalse = [] { return false; };
    wrapper->setDebugStateProvider(returnFalse, returnFalse);

    // Initialize graph node highlighter
    bbHighlighter = std::make_unique<BasicBlockHighlighter>();

    // Initialize Async tasks manager
    asyncTaskManager = new AsyncTaskManager(this);

    connect(wrapper, &RizinWrapper::reset, this, &CoreSession::refreshAll);

    // Show a message box if the wrapper logs any message
    connect(wrapper, &RizinWrapper::log, this,
            [](LogLevel level, const QString &title, const QString &text) {
                // cannot directly execute a message box here because background threads aren't
                // allowed to touch GUI. so we just drop an event
                QTimer::singleShot(0, qApp, [=]() {
                    switch (level) {
                    case LogLevel::Error:
                        QMessageBox::critical(nullptr, title, text);
                        break;
                    case LogLevel::Warning:
                        QMessageBox::warning(nullptr, title, text);
                        break;
                    case LogLevel::Info:
                        QMessageBox::information(nullptr, title, text);
                        break;
                    }
                });
            });

    // rz_core internally sets the cons_sleep callbacks each time it is created
    // so we have to overwrite each time
    initConsCallbacks();
    // TODO:
    sleepTask = wrapper->handleSleepBegin();
}

RizinLocked CoreSession::lock()
{
    return RizinLocked(this);
}

QDir CoreSession::getCutterRCDefaultDirectory() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
}

QVector<QString> CoreSession::getCutterRCFilePaths() const
{
    QVector<QString> result;
    result.push_back(QFileInfo(QDir::home(), ".cutterrc").absoluteFilePath());
    const QStringList locations =
            QStandardPaths::standardLocations(QStandardPaths::AppConfigLocation);
    for (auto &location : locations) {
        result.push_back(QFileInfo(QDir(location), ".cutterrc").absoluteFilePath());
    }
    result.push_back(QFileInfo(getCutterRCDefaultDirectory(), "rc")
                             .absoluteFilePath()); // File in config editor is from this path
    return result;
}

void CoreSession::initConsCallbacks()
{
    RzCons *cons = rz_cons_singleton();
    if (cons) {
        cons->cb_sleep_begin = CoreSession::consSleepBegin;
        cons->cb_sleep_end = CoreSession::consSleepEnd;
    }
}

void *CoreSession::consSleepBegin(void * /* user */)
{
    // TODO: checks
    /*The "Window Drag" Test (Verifies GUI Responsiveness)

    This verifies that QCoreApplication::processEvents is keeping Cutter alive on the main thread.

    How to verify:

        Load a moderately large binary.

        Force a long command to run on the main thread (e.g., type aaa in the console widget if it
    executes synchronously, or run a heavy Python script).

        While it is running, grab the edge of the Cutter window and resize it.

            If it works: The window resizes smoothly and repaints. processEvents is successfully
    unblocking the OS window manager.

            If it fails: The window turns white, lags heavily, or the OS shows a spinning beachball
    / "Not Responding" prompt.

        Try to click a button (like the "x" on a tab or a toolbar button).

            If it works: The click is completely ignored until the command finishes, proving
    ExcludeUserInputEvents is safely preventing nested operati
    */

    if (!currentActiveSession) {
        return nullptr;
    }

    qDebug() << ">>>>> consSleepBeign() triggerred <<<<<<";

    // TODO: add the processEvents thing??
    // Check if rizin calls this callback during long execution of command

    return currentActiveSession->wrapper->handleSleepBegin();
}

void CoreSession::consSleepEnd(void * /* user */, void *bed)
{
    if (!currentActiveSession || !bed) {
        return;
    }

    return currentActiveSession->wrapper->handleSleepEnd(bed);
}

void CoreSession::loadCutterRC()
{
    const auto result = getCutterRCFilePaths();
    for (auto &cutterRCFilePath : result) {
        auto cutterRCFileInfo = QFileInfo(cutterRCFilePath);
        if (!cutterRCFileInfo.exists() || !cutterRCFileInfo.isFile()) {
            continue;
        }
        qInfo() << tr("Loading initialization file from ") << cutterRCFilePath;
        LOCK(this, {
            rz_core_cmd_file(rizin.core(), cutterRCFilePath.toUtf8().constData());
            rz_cons_flush();
        });
    }
}

void CoreSession::loadDefaultCutterRC()
{
    auto cutterRCFilePath = QFileInfo(getCutterRCDefaultDirectory(), "rc").absoluteFilePath();
    const auto cutterRCFileInfo = QFileInfo(cutterRCFilePath);
    if (!cutterRCFileInfo.exists() || !cutterRCFileInfo.isFile()) {
        return;
    }
    qInfo() << tr("Loading initialization file from ") << cutterRCFilePath;
    auto rizin = this->lock();
    rz_core_cmd_file(rizin.core(), cutterRCFilePath.toUtf8().constData());
    rz_cons_flush();
}

bool CoreSession::asyncTask(std::function<void *(RzCore *)> fcn, std::shared_ptr<RizinTask> &task)
{
    if (task) {
        return false;
    }

    auto rizin = this->lock();
    const RVA offset = rizin->getOffset();
    task = std::shared_ptr<RizinTask>(new RizinFunctionTask(std::move(fcn), true));
    connect(task.get(), &RizinTask::finished, task.get(), [this, offset, task]() {
        auto rizin = this->lock();

        if (offset != rizin->getOffset()) {
            rizin->updateSeek();
        }
    });

    return true;
}

void CoreSession::functionTask(std::function<void *(RzCore *)> fcn)
{
    auto task = std::unique_ptr<RizinTask>(new RizinFunctionTask(std::move(fcn), true));
    task->startTask();
    task->joinTask();
}

QString CoreSession::cmdTask(const QString &str)
{
    RizinCmdTask task(str);
    task.startTask();
    task.joinTask();
    return task.getResult();
}

void CoreSession::showMemoryWidget()
{
    emit showMemoryWidgetRequested();
}

void CoreSession::seekAndShow(ut64 offset)
{
    {
        auto rizin = this->lock();
        rizin->seek(offset);
    }
    emit showAddressRequested(offset);
}

void CoreSession::seekAndShow(const QString &offset)
{
    RVA addr;
    {
        auto rizin = this->lock();
        rizin->seek(offset);
        addr = rizin->math(offset);
    }
    emit showAddressRequested(addr);
}

void CoreSession::setGraphEmpty(bool empty)
{
    emptyGraph = empty;
}

bool CoreSession::isGraphEmpty() const
{
    return emptyGraph;
}

void CoreSession::showTypeInTypesWidget(const QString &typeName)
{
    emit showTypeRequested(typeName);
}

void CoreSession::message(const QString &msg, bool debug)
{
    if (msg.isEmpty()) {
        return;
    }
    if (debug) {
        qDebug() << msg;
        emit newDebugMessage(msg);
        return;
    }
    emit newMessage(msg);
}

BasicBlockHighlighter *CoreSession::getBBHighlighter()
{
    return bbHighlighter.get();
}

BasicInstructionHighlighter *CoreSession::getBIHighlighter()
{
    return &biHighlighter;
}

void CoreSession::triggerRefreshAll()
{
    emit refreshAll();
}

void CoreSession::triggerAsmOptionsChanged()
{
    emit asmOptionsChanged();
}

void CoreSession::triggerGraphOptionsChanged()
{
    emit graphOptionsChanged();
}

void CoreSession::triggerDebugOptionsChanged()
{
    emit debugOptionsChanged();
}

void CoreSession::triggerAnalysisOptionsChanged()
{
    emit analysisOptionsChanged();
}

void CoreSession::triggerSymbolsOptionsChanged()
{
    emit symbolsOptionsChanged();
}

RizinLocked::RizinLocked(CoreSession *session) : session(session)
{
    lock();

    // TODO: test
    assert(session->lockDepth >= 0);
    session->lockDepth++;
    if (session->lockDepth == 1) {
        assert(session->sleepTask);
        session->consSleepEnd(nullptr, session->sleepTask);
        session->sleepTask = nullptr;
    }
}

RizinLocked::~RizinLocked()
{
    // TODO:
    CoreSession::currentActiveSession = session;
    assert(session->lockDepth > 0);
    session->lockDepth--;
    if (session->lockDepth == 0) {
        session->sleepTask = session->consSleepBegin(nullptr);
    }

    unlock();
}

void RizinLocked::unlock()
{
    if (locked) {
        session->mutex.unlock();
        locked = false;
    }
}

void RizinLocked::lock()
{
    if (!locked) {
        session->mutex.lock();
        locked = true;
    }
}

RzCore *RizinLocked::core()
{
    assert(locked && "Cannot access RzCore after unlock(), must lock() before");
    return session->wrapper->core;
}

RizinWrapper *RizinLocked::operator->() &
{
    assert(locked && "Cannot access RizinWrapper after unlock(), must lock() before");
    return session->wrapper;
}