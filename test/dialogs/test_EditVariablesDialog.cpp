/**
 * @file test_EditVariablesDialog.cpp
 * @brief Renaming and retyping a function's variables through their dialog.
 */

#include "Fixtures.h"
#include "TestEnv.h"
#include "UiHelpers.h"

#include "dialogs/EditVariablesDialog.h"

#include <QTest>

namespace {

bool hasVariable(RVA function, const QString &name)
{
    const QList<VariableDescription> variables = CutterTest::core()->getVariables(function);
    for (const VariableDescription &variable : variables) {
        if (variable.name == name) {
            return true;
        }
    }
    return false;
}

QString firstVariable(RVA function)
{
    const QList<VariableDescription> variables = CutterTest::core()->getVariables(function);
    return variables.isEmpty() ? QString() : variables.first().name;
}

/**
 * @brief Opens the dialog for function, renames variable to newName and accepts it.
 */
bool renameThroughDialog(RVA function, const QString &variable, const QString &newName)
{
    EditVariablesDialog dialog(function, variable);
    return CutterTest::setLineEditText(&dialog, "nameEdit", newName)
            && CutterTest::clickStandardButton(&dialog, QDialogButtonBox::Ok);
}

} // namespace

class TestEditVariablesDialog : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();

    void renameAppliesToVariable();
    void newNameIsSanitized();
    /**
     * @brief Editing a variable works when the seek is in another function.
     *
     * applyFields() looks up the function at the seek instead of the one the dialog was
     * opened for, so with the seek elsewhere OK does nothing. Red until fixed.
     */
    void editAppliesWhenSeekIsInAnotherFunction();

private:
    RVA function = RVA_INVALID;
    RVA otherFunction = RVA_INVALID;
};

void TestEditVariablesDialog::initTestCase()
{
    CUTTER_REQUIRE_FIXTURE(const QString path, CutterTest::Fixtures::helloX64());
    QVERIFY(CutterTest::openFile(path));
    CutterTest::analyze();

    const QList<FunctionDescription> functions = CutterTest::core()->getAllFunctions();
    for (const FunctionDescription &candidate : functions) {
        const bool hasVariables = !firstVariable(candidate.offset).isEmpty();
        if (function == RVA_INVALID && hasVariables) {
            function = candidate.offset;
        } else if (otherFunction == RVA_INVALID && !hasVariables) {
            otherFunction = candidate.offset;
        }

        if (function != RVA_INVALID && otherFunction != RVA_INVALID) {
            // found what we needed
            break;
        }
    }
    QVERIFY2(function != RVA_INVALID, "no analyzed function with variables");
    QVERIFY2(otherFunction != RVA_INVALID, "no analyzed function without variables");
}

void TestEditVariablesDialog::init()
{
    CutterTest::core()->seek(function);
}

/**
 * @brief A new name entered in the dialog renames the variable.
 */
void TestEditVariablesDialog::renameAppliesToVariable()
{
    const QString oldName = firstVariable(function);
    const QString newName = QStringLiteral("cutter_test_var");

    QVERIFY(renameThroughDialog(function, oldName, newName));
    QVERIFY(hasVariable(function, newName));
    QVERIFY(!hasVariable(function, oldName));
}

/**
 * @brief Characters rizin doesn't allow in a name are replaced.
 */
void TestEditVariablesDialog::newNameIsSanitized()
{
    const QString oldName = firstVariable(function);

    QVERIFY(renameThroughDialog(function, oldName, "cutter sanitized/name\\var"));
    QVERIFY(hasVariable(function, "cutter_sanitized_name_var"));
}

/**
 * @brief The edit applies to the dialog's function when the seek is in another function.
 *
 * To reproduce:
 * 1. Show the Decompiler on a function and unsync it.
 * 2. Seek to another function in the Disassembly.
 * 3. In the Decompiler, right click a variable → Edit variable, rename it, OK: nothing changed.
 */
void TestEditVariablesDialog::editAppliesWhenSeekIsInAnotherFunction()
{
    const QString oldName = firstVariable(function);
    const QString newName = QStringLiteral("cutter_test_var_elsewhere");
    CutterTest::core()->seek(otherFunction);

    QVERIFY(renameThroughDialog(function, oldName, newName));
    QVERIFY(hasVariable(function, newName));
    QVERIFY(!hasVariable(function, oldName));
}

CUTTER_CORE_TEST_MAIN(TestEditVariablesDialog)
#include "test_EditVariablesDialog.moc"
