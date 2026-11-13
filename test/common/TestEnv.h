#ifndef CUTTER_TEST_ENV_H
#define CUTTER_TEST_ENV_H

#include "core/Cutter.h"

#include <QString>
#include <QTest>

#include <memory>

class MainWindow;

namespace CutterTest {

constexpr int skipReturnCode = 77; // TODO: wat dis
inline constexpr char testbinsVariable[] = "CUTTER_TESTBINS_DIR";

QString fixturePath(const QString &relativePath);
const char *missingFixtureMessage();

CutterCore *core();
MainWindow *mainWindow();

bool openFile(const QString &path);
void analyze();
RVA addressWithoutComment(RVA start);

/**
 * @brief Sets up the layer's environment and runs the tests of test in it.
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
int runAppTest(std::unique_ptr<QObject> test, int &argc, char **argv, const QString &fixture);

} // namespace CutterTest

#define CUTTER_REQUIRE_FIXTURE(target, fixture)                                                    \
    if (CutterTest::fixturePath(fixture).isEmpty()) {                                              \
        QSKIP(CutterTest::missingFixtureMessage());                                                \
    }                                                                                              \
    target = CutterTest::fixturePath(fixture)

#define CUTTER_NO_CORE_TEST_MAIN(TestClass)                                                        \
    int main(int argc, char *argv[])                                                               \
    {                                                                                              \
        return CutterTest::runWithoutCore(std::make_unique<TestClass>(), argc, argv);              \
    }

#define CUTTER_CORE_TEST_MAIN(TestClass)                                                           \
    int main(int argc, char *argv[])                                                               \
    {                                                                                              \
        return CutterTest::runCoreTest(std::make_unique<TestClass>(), argc, argv);                 \
    }

#define CUTTER_APP_TEST_MAIN_WITH_FILE(TestClass, fixture)                                         \
    int main(int argc, char *argv[])                                                               \
    {                                                                                              \
        return CutterTest::runAppTest(std::make_unique<TestClass>(), argc, argv, fixture);         \
    }

#define CUTTER_APP_TEST_MAIN(TestClass) CUTTER_APP_TEST_MAIN_WITH_FILE(TestClass, QString())

#endif // CUTTER_TEST_ENV_H
