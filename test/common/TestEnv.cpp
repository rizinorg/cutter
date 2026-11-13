#include "TestEnv.h"

#include "IsolatedSettings.h"

#include "CutterApplication.h"
#include "common/AsyncTask.h"
#include "core/CutterDescriptions.h"
#include "core/MainWindow.h"

#include <QApplication>
#include <QByteArrayList>
#include <QDeadlineTimer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>

#include <cstdio>
#include <vector>

static void initCutterResources()
{
    Q_INIT_RESOURCE(resources);
}

namespace {

constexpr int fileLoadTimeoutMs = 120000;
constexpr RVA commentSearchRange = 0x100;

void prepareEnvironment()
{
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }

    initCutterResources();

    // TODO: why is this needed??
    qRegisterMetaType<QList<StringDescription>>();
    qRegisterMetaType<QList<FunctionDescription>>();

    CutterTest::isolateSettings();

    QSettings settings;
    settings.setValue("firstExecution", false);
    settings.sync();
}

bool waitForOpenedFile()
{
    const QDeadlineTimer deadline(fileLoadTimeoutMs);
    while (!deadline.hasExpired()) {
        MainWindow *main = CutterTest::mainWindow();
        if (main && main->isVisible()
            && !CutterTest::core()->getAsyncTaskManager()->getTasksRunning()) {
            return true;
        }
        QTest::qWait(50); // TODO: blocks processor? can we not yield?
    }
    return false;
}

/**
 * @brief Runs the tests QtTest selected in qInit, finishes QtTest's output and destroys the test.
 */
int runTests(std::unique_ptr<QObject> &test)
{
    const int result = QTest::qRun();
    QTest::qCleanup();
    test.reset();
    return result;
}

} // namespace

namespace CutterTest {

QString fixturePath(const QString &relativePath)
{
    const QString root = qEnvironmentVariable(testbinsVariable);
    if (root.isEmpty()) {
        return {};
    }
    const QFileInfo info(QDir(root).filePath(relativePath));
    return info.isFile() ? info.absoluteFilePath() : QString();
}

const char *missingFixtureMessage()
{
    static const QByteArray message =
            QByteArray("test file not available: re-run cmake with network access to download it, "
                       "or pass -D")
            + testbinsVariable + "=<path to your rizin-testbins clone> to cmake";
    return message.constData();
}

CutterCore *core()
{
    return CutterCore::instance();
}

MainWindow *mainWindow()
{
    auto *app = qobject_cast<CutterApplication *>(QCoreApplication::instance());
    return app ? app->getMainWindow() : nullptr;
}

bool openFile(const QString &path)
{
    // TODO: should have permission as param??
    return core()->loadFile(path, 0, 0, RZ_PERM_R, 1, true);
}

void analyze()
{
    // TODO: should take the analysis level
    core()->cmdRaw("aaa");
}

RVA addressWithoutComment(RVA start)
{
    // TODO: why is this in env? shouldn't this be in helpers?
    for (RVA candidate = start; candidate < start + commentSearchRange; ++candidate) {
        if (core()->getCommentAt(candidate).isEmpty()) {
            return candidate;
        }
    }
    return RVA_INVALID;
}

int runWithoutCore(std::unique_ptr<QObject> test, int &argc, char **argv)
{
    prepareEnvironment();
    QTest::qInit(test.get(), argc, argv);

    const QApplication app(argc, argv);

    return runTests(test);
}

int runCoreTest(std::unique_ptr<QObject> test, int &argc, char **argv)
{
    prepareEnvironment();
    QTest::qInit(test.get(), argc, argv);

    const QApplication app(argc, argv);
    CutterCore cutterCore;
    cutterCore.initialize(false);
    cutterCore.setSettings();

    return runTests(test);
}

// TODO: should we register an enum for this fixture? so no chances of wrong thing. Although it will
// log an error if the fixture name/path is incorrect
// TODO: maybe better name? because it auto analyzes via -A flag
int runAppTest(std::unique_ptr<QObject> test, int &argc, char **argv, const QString &fixture)
{
    prepareEnvironment();

    QString file;
    if (!fixture.isEmpty()) {
        file = fixturePath(fixture);
        if (file.isEmpty()) {
            std::fprintf(stderr, "SKIP: fixture '%s' not found: %s\n", qPrintable(fixture),
                         missingFixtureMessage());
            return skipReturnCode;
        }
    }

    QTest::qInit(test.get(), argc, argv);

    QByteArrayList cutterArgs = { QByteArray(argv[0]), "--no-output-redirect",
                                  "--no-cutter-plugins", "--no-rizin-plugins" };
    if (!file.isEmpty()) {
        cutterArgs << "-A" << "1" << QFile::encodeName(file);
    }
    std::vector<char *> cutterArgv;
    for (QByteArray &arg : cutterArgs) {
        cutterArgv.push_back(arg.data());
    }
    cutterArgv.push_back(nullptr);
    int cutterArgc = static_cast<int>(cutterArgs.size());

    const CutterApplication app(cutterArgc, cutterArgv.data());

    if (!file.isEmpty() && !waitForOpenedFile()) {
        std::fprintf(stderr, "FAIL: '%s' did not finish loading\n", qPrintable(file));
        QTest::qCleanup();
        test.reset();
        return 1;
    }

    return runTests(test);
}

} // namespace CutterTest
