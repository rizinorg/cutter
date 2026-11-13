/**
 * @file test_FlagDialog.cpp
 * @brief Editing an existing flag through the flag dialog.
 */

#include "Fixtures.h"
#include "TestEnv.h"
#include "UiHelpers.h"

#include "dialogs/FlagDialog.h"

#include <QTest>

namespace {

constexpr RVA flagSearchRange = 0x100;

/**
 * @brief The first address from start that has no flag.
 */
RVA addressWithoutFlag(RVA start)
{
    for (RVA candidate = start; candidate < start + flagSearchRange; ++candidate) {
        if (CutterTest::core()->listFlagsAsStringAt(candidate).isEmpty()) {
            return candidate;
        }
    }
    return RVA_INVALID;
}

RVA flagSize(const QString &name)
{
    const QList<FlagDescription> flags = CutterTest::core()->getAllFlags();
    for (const FlagDescription &flag : flags) {
        if (flag.name == name) {
            return flag.size;
        }
    }
    return RVA_INVALID;
}

QString fieldText(FlagDialog &dialog, const char *objectName)
{
    auto *edit = dialog.findChild<QLineEdit *>(objectName);
    return edit ? edit->text() : QString();
}

} // namespace

class TestFlagDialog : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    /**
     * @brief Editing a flag shows its size and applies a new one.
     */
    void editingFlagAppliesNewSize();
    /**
     * @brief Renaming replaces ';' and '@' in the name, like adding a flag does.
     */
    void renameIsSanitizedLikeAdd();

private:
    RVA address = RVA_INVALID;
    const QString flagName = QStringLiteral("cutter_test_flag");
    const QString renamedFlag = QStringLiteral("cutter_test_renamed");
};

void TestFlagDialog::initTestCase()
{
    CUTTER_REQUIRE_FIXTURE(const QString path, CutterTest::Fixtures::helloX64());
    QVERIFY(CutterTest::openFile(path));

    address = addressWithoutFlag(CutterTest::core()->getOffset());
    QVERIFY2(address != RVA_INVALID, "no address without a flag near the current seek");
}

void TestFlagDialog::init()
{
    CutterTest::core()->addFlag(address, flagName, 4);
}

void TestFlagDialog::cleanup()
{
    CutterTest::core()->delFlag(flagName);
    CutterTest::core()->delFlag(renamedFlag);
}

/**
 * @brief The dialog shows the flag's size, and a new size is applied.
 *
 * To reproduce:
 * 1. Right click a flagged address → Edit flag: the size field was empty.
 * 2. Enter a new size and press OK: the size didn't change.
 */
void TestFlagDialog::editingFlagAppliesNewSize()
{
    FlagDialog dialog(address);
    QCOMPARE(fieldText(dialog, "sizeEdit"), QString("4"));

    QVERIFY(CutterTest::setLineEditText(&dialog, "sizeEdit", "16"));
    QVERIFY(CutterTest::clickStandardButton(&dialog, QDialogButtonBox::Ok));

    QCOMPARE(flagSize(flagName), RVA(16));
}

/**
 * @brief Renaming a flag replaces the characters that adding a flag replaces.
 *
 * To reproduce:
 * 1. Right click a flagged address → Edit flag.
 * 2. Rename it to "a;b@c": the name kept ';' and '@', which rizin commands split on.
 */
void TestFlagDialog::renameIsSanitizedLikeAdd()
{
    FlagDialog dialog(address);
    QVERIFY(CutterTest::setLineEditText(&dialog, "nameEdit", "cutter;test@renamed"));
    QVERIFY(CutterTest::clickStandardButton(&dialog, QDialogButtonBox::Ok));

    QCOMPARE(flagSize(flagName), RVA_INVALID);
    QCOMPARE(flagSize(renamedFlag), RVA(4));
}

CUTTER_CORE_TEST_MAIN(TestFlagDialog)
#include "test_FlagDialog.moc"
