/**
 * @file test_EditMethodDialog.cpp
 * @brief Editing a class method through Edit Method.
 */

#include "Fixtures.h"
#include "TestEnv.h"
#include "UiHelpers.h"

#include "dialogs/EditMethodDialog.h"

#include <QComboBox>
#include <QTest>

namespace {

bool hasMethod(const QString &className, const QString &methodName)
{
    for (const AnalysisMethodDescription &method :
         CutterTest::core()->getAnalysisClassMethods(className)) {
        if (method.name == methodName) {
            return true;
        }
    }
    return false;
}

} // namespace

class TestEditMethodDialog : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void editMethodMovesToChosenClass();

private:
    const QString sourceClass = QStringLiteral("CutterTestSource");
    const QString targetClass = QStringLiteral("CutterTestTarget");
};

void TestEditMethodDialog::initTestCase()
{
    CUTTER_REQUIRE_FIXTURE(const QString path, CutterTest::Fixtures::helloX64());
    QVERIFY(CutterTest::openFile(path));
    CutterTest::core()->createNewClass(sourceClass);
    CutterTest::core()->createNewClass(targetClass);
    const QList<QString> classes = CutterTest::core()->getAllAnalysisClasses(false);
    QVERIFY(classes.contains(sourceClass) && classes.contains(targetClass));
}

/**
 * @brief Choosing another class in Edit Method moves the method there.
 *
 * To reproduce:
 * 1. Windows → Info → Classes, with two classes.
 * 2. Right click a method → Edit Method, pick the other class and press OK: the method stayed in
 *    its class.
 */
void TestEditMethodDialog::editMethodMovesToChosenClass()
{
    AnalysisMethodDescription desc;
    desc.name = QStringLiteral("cutter_moved_method");
    desc.realName = desc.name;
    desc.addr = CutterTest::core()->getOffset();
    desc.vtableOffset = RVA_INVALID;
    CutterTest::core()->setAnalysisMethod(sourceClass, desc);
    QVERIFY(hasMethod(sourceClass, desc.name));

    QObject context;
    CutterTest::onceVisible<EditMethodDialog>(&context, [&](EditMethodDialog *dialog) {
        if (auto *classes = dialog->findChild<QComboBox *>()) {
            classes->setCurrentIndex(classes->findData(targetClass));
        }
        CutterTest::clickStandardButton(dialog, QDialogButtonBox::Ok);
    });
    EditMethodDialog::editMethod(sourceClass, desc.name, nullptr);

    QVERIFY2(hasMethod(targetClass, desc.name), "the method is not in the chosen class");
    QVERIFY2(!hasMethod(sourceClass, desc.name), "the method stayed in the original class");
}

CUTTER_CORE_TEST_MAIN(TestEditMethodDialog)
#include "test_EditMethodDialog.moc"
