/**
 * @file test_CoreSignals.cpp
 * @brief The signals the core's edit operations emit.
 */

#include "Fixtures.h"
#include "TestEnv.h"

#include <QScopeGuard>
#include <QSignalSpy>
#include <QTest>

namespace {

bool hasFlag(CutterCore *core, RVA address, const QString &name)
{
    const QStringList flags = core->listFlagsAsStringAt(address).split(',', Qt::SkipEmptyParts);
    for (const QString &flag : flags) {
        if (flag.trimmed() == name) {
            return true;
        }
    }
    return false;
}

bool hasVariable(CutterCore *core, RVA function, const QString &name)
{
    const QList<VariableDescription> variables = core->getVariables(function);
    for (const VariableDescription &variable : variables) {
        if (variable.name == name) {
            return true;
        }
    }
    return false;
}

/**
 * @brief The name of the global variable at address, or an empty string.
 */
QString globalNameAt(CutterCore *core, RVA address)
{
    const QList<GlobalDescription> globals = core->getAllGlobals();
    for (const GlobalDescription &global : globals) {
        if (global.addr == address) {
            return global.name;
        }
    }
    return {};
}

} // namespace

class TestCoreSignals : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void commentEditsNotifyOnceWithAddress();
    void renameFunctionNotifiesOnceWithNewName();
    void renameFunctionWithoutFunctionNotifiesNothing();
    void renameVariableNotifiesCodeViewsOnce();
    void flagEditsNotifyOnce();
    void renameGlobalVariableKeepsTypeAndNotifiesOnce();
    void byteEditNotifiesInstructionChangedWithAddress();
    void undefineFunctionNotifiesFunctionsChanged();

private:
    RVA freeAddress = RVA_INVALID;
    RVA renameTarget = RVA_INVALID;
    RVA variableFunction = RVA_INVALID;
    RVA undefineTarget = RVA_INVALID;
};

void TestCoreSignals::initTestCase()
{
    CUTTER_REQUIRE_FIXTURE(const QString path, CutterTest::Fixtures::helloX64());
    QVERIFY(CutterTest::openFile(path));
    CutterTest::analyze();

    CutterCore *core = CutterTest::core();
    const RVA start = core->getOffset();
    QVERIFY(start != RVA_INVALID);

    freeAddress = CutterTest::addressWithoutComment(start);
    QVERIFY2(freeAddress != RVA_INVALID, "no address without a comment near the current seek");

    const QList<FunctionDescription> functions = core->getAllFunctions();
    for (const FunctionDescription &function : functions) {
        if (variableFunction == RVA_INVALID && !core->getVariables(function.offset).isEmpty()) {
            variableFunction = function.offset;
        }
    }
    QVERIFY2(variableFunction != RVA_INVALID, "no analyzed function with variables");

    for (const FunctionDescription &function : functions) {
        if (function.offset == variableFunction) {
            continue;
        }
        if (renameTarget == RVA_INVALID) {
            renameTarget = function.offset;
        } else if (undefineTarget == RVA_INVALID) {
            undefineTarget = function.offset;
        }
    }
    QVERIFY2(renameTarget != RVA_INVALID && undefineTarget != RVA_INVALID,
             "fixture needs at least three analyzed functions");
}

/**
 * @brief Setting and deleting a comment each emit commentsChanged once with its address.
 */
void TestCoreSignals::commentEditsNotifyOnceWithAddress()
{
    CutterCore *core = CutterTest::core();
    QVERIFY(core->getCommentAt(freeAddress).isEmpty());

    QSignalSpy spy(core, &CutterCore::commentsChanged);

    core->setComment(freeAddress, "first");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toULongLong(), freeAddress);
    QCOMPARE(core->getCommentAt(freeAddress), QString("first"));

    core->setComment(freeAddress, "second");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toULongLong(), freeAddress);
    QCOMPARE(core->getCommentAt(freeAddress), QString("second"));

    core->delComment(freeAddress);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toULongLong(), freeAddress);
    QVERIFY(core->getCommentAt(freeAddress).isEmpty());
}

/**
 * @brief Renaming a function emits functionRenamed once with the name rizin stored.
 *
 * To reproduce:
 * 1. Right click a function in Functions → Rename, and enter "  myname".
 * 2. Views updated from the signal showed "  myname"; the function is named "myname".
 */
void TestCoreSignals::renameFunctionNotifiesOnceWithNewName()
{
    CutterCore *core = CutterTest::core();
    const QString newName = QStringLiteral("cutter_test_renamed_function");

    QSignalSpy spy(core, &CutterCore::functionRenamed);

    core->renameFunction(renameTarget, newName);
    QCOMPARE(spy.count(), 1);
    const QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toULongLong(), renameTarget);
    QCOMPARE(arguments.at(1).toString(), newName);

    RzAnalysisFunction *function = core->functionAt(renameTarget);
    QVERIFY(function);
    QCOMPARE(QString::fromUtf8(function->name), newName);
}

/**
 * @brief Renaming at an address without a function emits nothing.
 *
 * Every view used to refresh as if a function had been renamed.
 */
void TestCoreSignals::renameFunctionWithoutFunctionNotifiesNothing()
{
    CutterCore *core = CutterTest::core();
    const RVA notAFunction = renameTarget + 1;
    QVERIFY(!core->functionAt(notAFunction));

    QSignalSpy spy(core, &CutterCore::functionRenamed);

    core->renameFunction(notAFunction, QStringLiteral("cutter_test_missing_function"));
    QCOMPARE(spy.count(), 0);
}

/**
 * @brief Renaming a variable emits refreshCodeViews once.
 */
void TestCoreSignals::renameVariableNotifiesCodeViewsOnce()
{
    CutterCore *core = CutterTest::core();
    const QList<VariableDescription> variables = core->getVariables(variableFunction);
    QVERIFY(!variables.isEmpty());
    const QString oldName = variables.first().name;
    const QString newName = QStringLiteral("cutter_test_var");
    QVERIFY(!hasVariable(core, variableFunction, newName));

    QSignalSpy spy(core, &CutterCore::refreshCodeViews);

    core->renameFunctionVariable(newName, oldName, variableFunction);
    QCOMPARE(spy.count(), 1);
    QVERIFY(hasVariable(core, variableFunction, newName));
    QVERIFY(!hasVariable(core, variableFunction, oldName));
}

/**
 * @brief Adding, renaming and deleting a flag each emit flagsChanged once.
 */
void TestCoreSignals::flagEditsNotifyOnce()
{
    CutterCore *core = CutterTest::core();
    const QString name = QStringLiteral("cutter_test_flag_a");
    const QString renamed = QStringLiteral("cutter_test_renamed");
    QVERIFY(!hasFlag(core, freeAddress, name));
    QVERIFY(!hasFlag(core, freeAddress, renamed));

    QSignalSpy spy(core, &CutterCore::flagsChanged);

    core->addFlag(freeAddress, name, 1);
    QCOMPARE(spy.count(), 1);
    QVERIFY(hasFlag(core, freeAddress, name));

    core->renameFlag(name, renamed);
    QCOMPARE(spy.count(), 2);
    QVERIFY(hasFlag(core, freeAddress, renamed));
    QVERIFY(!hasFlag(core, freeAddress, name));

    core->renameFlag(QStringLiteral("cutter_test_missing_flag"), QStringLiteral("unused"));
    QCOMPARE(spy.count(), 2);

    core->delFlag(renamed);
    QCOMPARE(spy.count(), 3);
    QVERIFY(!hasFlag(core, freeAddress, renamed));
}

/**
 * @brief Renaming a global variable keeps its type and emits globalVarsChanged once.
 *
 * To reproduce:
 * 1. Windows → Info → Globals, right click a global → Edit.
 * 2. Change only the name and press OK: a heap use after free under ASan.
 */
void TestCoreSignals::renameGlobalVariableKeepsTypeAndNotifiesOnce()
{
    CutterCore *core = CutterTest::core();
    const QString name = QStringLiteral("cutter_test_global");
    const QString renamed = QStringLiteral("cutter_test_global_renamed");
    const QString type = QStringLiteral("int");

    QVERIFY(globalNameAt(core, freeAddress).isEmpty());
    core->addGlobalVariable(freeAddress, name, type);
    const auto removeGlobal = qScopeGuard([&]() { core->delGlobalVariable(freeAddress); });
    QCOMPARE(globalNameAt(core, freeAddress), name);

    QSignalSpy spy(core, &CutterCore::globalVarsChanged);
    core->modifyGlobalVariable(freeAddress, renamed, type);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(globalNameAt(core, freeAddress), renamed);
    QCOMPARE(core->getGlobalVariableType(freeAddress), type);
}

/**
 * @brief Editing bytes emits instructionChanged with the edited address.
 */
void TestCoreSignals::byteEditNotifiesInstructionChangedWithAddress()
{
    CutterCore *core = CutterTest::core();
    core->setIOCache(true);
    QVERIFY(core->isIOCacheEnabled());

    const QByteArray original = core->ioRead(freeAddress, 1);
    QCOMPARE(original.size(), 1);
    const QByteArray patched = QByteArray(1, static_cast<char>(original.at(0) ^ 0xff));

    QSignalSpy spy(core, &CutterCore::instructionChanged);

    core->editBytes(freeAddress, QString::fromLatin1(patched.toHex()));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toULongLong(), freeAddress);
    QCOMPARE(core->ioRead(freeAddress, 1), patched);

    core->editBytes(freeAddress, QString::fromLatin1(original.toHex()));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toULongLong(), freeAddress);
    QCOMPARE(core->ioRead(freeAddress, 1), original);
}

/**
 * @brief Undefining a function emits functionsChanged.
 */
void TestCoreSignals::undefineFunctionNotifiesFunctionsChanged()
{
    CutterCore *core = CutterTest::core();
    QVERIFY(core->functionAt(undefineTarget));

    QSignalSpy spy(core, &CutterCore::functionsChanged);

    core->delFunction(undefineTarget);
    QCOMPARE(spy.count(), 1);
    QVERIFY(!core->functionAt(undefineTarget));
}

CUTTER_CORE_TEST_MAIN(TestCoreSignals)
#include "test_CoreSignals.moc"
