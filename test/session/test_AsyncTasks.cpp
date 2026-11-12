/**
 * @file test_AsyncTasks.cpp
 * @brief Background tasks run through the core's AsyncTaskManager.
 */

#include "CallbackTask.h"
#include "Fixtures.h"
#include "TestEnv.h"
#include "common/AnalysisTask.h"

#include <QCoreApplication>
#include <QMutex>
#include <QMutexLocker>
#include <QScopeGuard>
#include <QSemaphore>
#include <QSignalSpy>
#include <QTest>
#include <QThread>

#include <atomic>
#include <memory>

namespace {

constexpr int taskTimeoutMs = 5000;
constexpr int duplicateSignalWaitMs = 100;
constexpr int queuedWaitMs = 200;
constexpr int analysisStartTimeoutMs = 60000;
constexpr int interruptTimeoutMs = 2000;

struct Gate
{
    QSemaphore semaphore;
    std::atomic<int> started { 0 };
};

/**
 * @brief A task that runs until its gate is released.
 */
std::shared_ptr<CutterTest::CallbackTask> gatedTask(const std::shared_ptr<Gate> &gate)
{
    return std::make_shared<CutterTest::CallbackTask>([gate]() {
        ++gate->started;
        gate->semaphore.acquire();
    });
}

AsyncTaskManager *manager()
{
    return CutterTest::core()->getAsyncTaskManager();
}

} // namespace

class TestAsyncTasks : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    /**
     * @brief One task goes start -> running -> finished exactly once.
     */
    void taskReportsFinishedOnceAndLeavesManager();

    /**
     * @brief tasksChanged is always emitted on the GUI thread.
     *
     * A direct-connected lambda runs on the emitting thread and records it. The finish emit
     * sits next to tasks.removeOne(), so a background-thread emit means the task list was changed
     * off the GUI thread while the GUI reads it.
     */
    void tasksChangedArrivesOnGuiThread();

    /**
     * @brief The manager reports running until its last task finishes.
     */
    void managerRunsUntilEveryTaskFinished();

    /**
     * @brief The manager owns a running task even after the caller drops it.
     */
    void managerKeepsTaskAliveUntilFinished();

    /**
     * @brief wait() blocks while a task is queued and has not started.
     *
     * Every background thread is filled with gated tasks so the next one cannot run because there
     * is no room, hence stays in queued state. wait() must time out until the gated tasks finish.
     *
     * wait() returning early means it isn't handled correctly on queued tasks.
     */
    void waitBlocksWhileTaskIsQueued();

    /**
     * @brief Interrupting a real analysis finishes it quickly and cleanly.
     */
    void interruptedAnalysisFinishesQuickly();
};

void TestAsyncTasks::init()
{
    QVERIFY(!manager()->getTasksRunning());
}

void TestAsyncTasks::cleanup()
{
    QTRY_VERIFY_WITH_TIMEOUT(!manager()->getTasksRunning(), taskTimeoutMs);
}

/**
 * @brief A task emits finished once and is removed from the manager's list.
 *
 * A task reported twice runs finalizeOpen twice.
 */
void TestAsyncTasks::taskReportsFinishedOnceAndLeavesManager()
{
    auto gate = std::make_shared<Gate>();
    auto task = gatedTask(gate);
    const auto release = qScopeGuard([gate]() { gate->semaphore.release(); });

    QObject context;
    int finishedCount = 0;
    connect(task.get(), &AsyncTask::finished, &context, [&]() { ++finishedCount; });
    QSignalSpy tasksSpy(manager(), &AsyncTaskManager::tasksChanged);

    manager()->start(task);
    QVERIFY(manager()->getTasksRunning());
    QTRY_COMPARE_WITH_TIMEOUT(gate->started.load(), 1, taskTimeoutMs);
    QCOMPARE(finishedCount, 0);

    gate->semaphore.release(); // release so task can finish
    QTRY_COMPARE_WITH_TIMEOUT(finishedCount, 1, taskTimeoutMs);
    QTRY_VERIFY_WITH_TIMEOUT(!manager()->getTasksRunning(), taskTimeoutMs);

    QTest::qWait(duplicateSignalWaitMs);
    QCOMPARE(finishedCount, 1);
    QCOMPARE(tasksSpy.count(), 2);
}

/**
 * @brief tasksChanged is delivered on the GUI thread, also for a task finishing on a pool thread.
 */
void TestAsyncTasks::tasksChangedArrivesOnGuiThread()
{
    auto gate = std::make_shared<Gate>();
    auto task = gatedTask(gate);
    const auto release = qScopeGuard([gate]() { gate->semaphore.release(); });

    QMutex mutex;
    QList<QThread *> threads;
    QObject context;
    connect(
            manager(), &AsyncTaskManager::tasksChanged, &context,
            [&]() {
                const QMutexLocker locker(&mutex);
                threads.append(QThread::currentThread());
            },
            Qt::DirectConnection);

    manager()->start(task);
    gate->semaphore.release();
    QTRY_VERIFY_WITH_TIMEOUT(!manager()->getTasksRunning(), taskTimeoutMs);

    const QMutexLocker locker(&mutex);
    QVERIFY(!threads.isEmpty());
    for (QThread *thread : std::as_const(threads)) {
        QCOMPARE(thread, QCoreApplication::instance()->thread());
    }
}

/**
 * @brief The manager reports running until the last of several tasks finishes.
 */
void TestAsyncTasks::managerRunsUntilEveryTaskFinished()
{
    auto firstGate = std::make_shared<Gate>();
    auto secondGate = std::make_shared<Gate>();
    auto first = gatedTask(firstGate);
    auto second = gatedTask(secondGate);
    const auto release = qScopeGuard([firstGate, secondGate]() {
        firstGate->semaphore.release();
        secondGate->semaphore.release();
    });

    QObject context;
    bool firstFinished = false;
    connect(first.get(), &AsyncTask::finished, &context, [&]() { firstFinished = true; });

    manager()->start(first);
    manager()->start(second);
    QTRY_COMPARE_WITH_TIMEOUT(firstGate->started.load(), 1, taskTimeoutMs);

    firstGate->semaphore.release();
    QTRY_VERIFY_WITH_TIMEOUT(firstFinished, taskTimeoutMs);
    QTest::qWait(duplicateSignalWaitMs);
    QVERIFY(manager()->getTasksRunning());

    secondGate->semaphore.release();
    QTRY_VERIFY_WITH_TIMEOUT(!manager()->getTasksRunning(), taskTimeoutMs);
}

/**
 * @brief The manager keeps a started task alive until it finishes.
 */
void TestAsyncTasks::managerKeepsTaskAliveUntilFinished()
{
    auto gate = std::make_shared<Gate>();
    auto task = gatedTask(gate);
    const std::weak_ptr<AsyncTask> observer = task;
    const auto release = qScopeGuard([gate]() { gate->semaphore.release(); });

    manager()->start(task);
    QTRY_COMPARE_WITH_TIMEOUT(gate->started.load(), 1, taskTimeoutMs);
    task.reset();
    QVERIFY(!observer.expired());

    gate->semaphore.release();
    QTRY_VERIFY_WITH_TIMEOUT(!manager()->getTasksRunning(), taskTimeoutMs);
    QTRY_VERIFY_WITH_TIMEOUT(observer.expired(), taskTimeoutMs);
}

/**
 * @brief wait() on a task that is still queued blocks until it ran.
 *
 * Every pool thread is kept busy so the task stays queued. wait() used to return before the task
 * started, which let FunctionsWidget and StringsWidget run two refreshes at once. Not reproducible
 * from the UI.
 */
void TestAsyncTasks::waitBlocksWhileTaskIsQueued()
{
    const int poolSize = QThread::idealThreadCount();
    auto blockers = std::make_shared<Gate>();
    const auto release =
            qScopeGuard([blockers, poolSize]() { blockers->semaphore.release(poolSize); });
    for (int i = 0; i < poolSize; ++i) {
        manager()->start(gatedTask(blockers));
    }
    QTRY_COMPARE_WITH_TIMEOUT(blockers->started.load(), poolSize, taskTimeoutMs);

    auto ran = std::make_shared<std::atomic<bool>>(false);
    auto queued = std::make_shared<CutterTest::CallbackTask>([ran]() { *ran = true; });
    manager()->start(queued);

    const bool returned = queued->wait(queuedWaitMs);

    // shouldn't fail unless idead thread count in manager is manually changed to different value
    QVERIFY2(!ran->load(), "queued task ran: the pool was not full");

    QVERIFY2(!returned, "wait() returned while the task was still queued");

    blockers->semaphore.release(poolSize);
    QVERIFY(queued->wait(taskTimeoutMs));
    QVERIFY(ran->load());
}

/**
 * @brief Interrupting a running analysis of a large binary ends the task quickly.
 *
 * To reproduce:
 * 1. Open a large binary (e.g. /bin/ls) with "Auto analysis (experimental)".
 * 2. Cancel in the progress dialog: Cutter stayed frozen until the analysis finished.
 */
void TestAsyncTasks::interruptedAnalysisFinishesQuickly()
{
    CUTTER_REQUIRE_FIXTURE(const QString path, CutterTest::Fixtures::coreutilsLs());

    const CommandDescription analysis = { "aaaa", "Auto analysis (experimental)" };
    InitialOptions options;
    options.filename = path;
    options.endian = InitialOptions::Endianness::Auto;
    options.analysisCmd = { analysis };

    auto task = std::make_shared<AnalysisTask>();
    task->setOptions(options);

    QObject context;
    bool analysisStarted = false;
    bool finished = false;
    connect(task.get(), &AsyncTask::logChanged, &context, [&](const QString &log) {
        analysisStarted = analysisStarted || log.contains(analysis.translatedDescription());
    });
    connect(task.get(), &AsyncTask::finished, &context, [&]() { finished = true; });
    const auto stopOnEarlyExit = qScopeGuard([&]() {
        if (!finished) {
            task->interrupt();
            task->wait();
        }
    });

    manager()->start(task);
    QTRY_VERIFY_WITH_TIMEOUT(analysisStarted, analysisStartTimeoutMs);
    if (finished) {
        QSKIP("analysis finished before it could be interrupted");
    }

    task->interrupt();
    QTRY_VERIFY_WITH_TIMEOUT(finished, interruptTimeoutMs);
    QVERIFY(!task->getOpenFileFailed());
}

CUTTER_CORE_TEST_MAIN(TestAsyncTasks)
#include "test_AsyncTasks.moc"
