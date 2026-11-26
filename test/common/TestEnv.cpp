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

void prepareEnvironment()
{
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }

    initCutterResources();

    // TODO: Same as src/main. we should clean this up in both places
    // (PremadeS)
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
        QTest::qWait(50);
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
    return core()->loadFile(path, 0, 0, RZ_PERM_R, 1, true);
}

void analyze()
{
    core()->cmdRaw("aaa");
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
