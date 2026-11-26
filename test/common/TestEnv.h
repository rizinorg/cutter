#ifndef CUTTER_TEST_ENV_H
#define CUTTER_TEST_ENV_H

#include "IsolatedSettings.h"
#include "core/Cutter.h"

#include <QString>
#include <QTest>

#include <memory>

class MainWindow;

namespace CutterTest {

constexpr int skipReturnCode = 77;
inline constexpr char testbinsVariable[] = "CUTTER_TESTBINS_DIR";

QString binPath(const QString &relativePath);
const char *missingBinMessage();

CutterCore *core();
MainWindow *mainWindow();

bool openFile(const QString &path);
void analyze();

/**
 * @brief Sets up the environment, then runs the test methods of the given test object in it.
 *
 * The arguments go to QtTest first. QtTest handles the options that only print something, like
 * -functions, and exits right there. So listing a test's methods doesn't start Cutter or load a
 * binary. The environment is created only when tests are going to run.
 *
 * The test object is destroyed before the app and the core. A test can own widgets that are
 * also children of the main window, and those must go first.
 */
int runWithoutCore(std::unique_ptr<QObject> test, int &argc, char **argv);
int runCoreTest(std::unique_ptr<QObject> test, int &argc, char **argv);
int runAppTest(std::unique_ptr<QObject> test, int &argc, char **argv, const QString &bin);

} // namespace CutterTest

#define CUTTER_REQUIRE_BIN(target, bin)                                                            \
    if (CutterTest::binPath(bin).isEmpty()) {                                                      \
        QSKIP(CutterTest::missingBinMessage());                                                    \
    }                                                                                              \
    target = CutterTest::binPath(bin)

// Unit test: plain QApplication with isolated settings, no Cutter resources, core or window.
#define CUTTER_UNIT_TEST_MAIN(TestClass)                                                           \
    int main(int argc, char *argv[])                                                               \
    {                                                                                              \
        CutterTest::isolateSettings();                                                             \
        QApplication app(argc, argv);                                                              \
        TestClass test;                                                                            \
        return QTest::qExec(&test, argc, argv);                                                    \
    }

// Cutter environment (resources, isolated settings) and a QApplication, but no core.
#define CUTTER_NO_CORE_TEST_MAIN(TestClass)                                                        \
    int main(int argc, char *argv[])                                                               \
    {                                                                                              \
        return CutterTest::runWithoutCore(std::make_unique<TestClass>(), argc, argv);              \
    }

// Cutter environment plus an initialized CutterCore, but no main window.
#define CUTTER_CORE_TEST_MAIN(TestClass)                                                           \
    int main(int argc, char *argv[])                                                               \
    {                                                                                              \
        return CutterTest::runCoreTest(std::make_unique<TestClass>(), argc, argv);                 \
    }

// Full Cutter app with its main window, with the test binary opened and analyzed (aaa).
#define CUTTER_APP_TEST_MAIN_WITH_FILE(TestClass, bin)                                             \
    int main(int argc, char *argv[])                                                               \
    {                                                                                              \
        return CutterTest::runAppTest(std::make_unique<TestClass>(), argc, argv, bin);             \
    }

// Full Cutter app with its main window and no file open.
#define CUTTER_APP_TEST_MAIN(TestClass) CUTTER_APP_TEST_MAIN_WITH_FILE(TestClass, QString())

#endif // CUTTER_TEST_ENV_H
