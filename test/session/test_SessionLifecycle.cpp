/**
 * @file test_SessionLifecycle.cpp
 * @brief Tearing down a core while its background work is still running.
 */

#include "Fixtures.h"
#include "TestEnv.h"

#include "common/AnalysisTask.h"

#include <QElapsedTimer>
#include <QScopeGuard>
#include <QTest>

#include <atomic>
#include <memory>

namespace {

constexpr int analysisStartTimeoutMs = 60000;
constexpr qint64 teardownTimeoutMs = 5000;

} // namespace

class TestSessionLifecycle : public QObject
{
    Q_OBJECT

private slots:
    void destroyingCoreStopsRunningAnalysisFirst();
};

/**
 * @brief Destroying the core stops the running analysis before freeing rizin.
 *
 * To reproduce:
 * 1. Open a large binary (e.g. /bin/ls) with "Auto analysis (experimental)".
 * 2. Close Cutter during the analysis: the analysis kept using the freed rizin and crashed (seen
 *    under ASan).
 */
void TestSessionLifecycle::destroyingCoreStopsRunningAnalysisFirst()
{
    CUTTER_REQUIRE_FIXTURE(const QString path, CutterTest::Fixtures::coreutilsLs());

    auto core = std::make_unique<CutterCore>();
    core->initialize(false);
    core->setSettings();

    InitialOptions options;
    options.filename = path;
    options.endian = InitialOptions::Endianness::Auto;
    options.analysisCmd = { { "aaaa", "Auto analysis (experimental)" } };

    std::atomic<bool> finished { false };
    auto task = std::make_shared<AnalysisTask>();
    task->setOptions(options);
    connect(
            task.get(), &AsyncTask::finished, task.get(), [&finished]() { finished = true; },
            Qt::DirectConnection);

    QObject context;
    bool analysisStarted = false;
    connect(task.get(), &AsyncTask::logChanged, &context, [&](const QString &log) {
        analysisStarted = analysisStarted || log.contains("Executing analysis");
    });

    const auto stopOnEarlyExit = qScopeGuard([&]() {
        if (core) {
            task->interrupt();
            task->wait();
        }
    });
    core->getAsyncTaskManager()->start(task);
    QTRY_VERIFY_WITH_TIMEOUT(analysisStarted, analysisStartTimeoutMs);
    if (finished) {
        QSKIP("analysis finished before the core could be destroyed");
    }

    QElapsedTimer teardown;
    teardown.start();
    core.reset();

    QVERIFY(finished);
    QVERIFY2(teardown.elapsed() < teardownTimeoutMs,
             "destroying the core waited for the analysis instead of interrupting it");
}

CUTTER_NO_CORE_TEST_MAIN(TestSessionLifecycle)
#include "test_SessionLifecycle.moc"
