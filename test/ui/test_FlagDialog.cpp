/**
 * @file test_FlagDialog.cpp
 * @brief Editing an existing flag through the flag dialog.
 *
 * Covers: FlagDialog.
 */

#include "TestBins.h"
#include "TestEnv.h"
#include "UiHelpers.h"

#include "dialogs/FlagDialog.h"

#include <QTest>

namespace {

constexpr RVA flagSearchRange = 0x100;

/**
 * @brief The first address at or after start with no flag, or RVA_INVALID if none is found.
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

    void editingFlagAppliesNewSize();
    void renameIsSanitizedLikeAdd();

private:
    RVA address = RVA_INVALID;
    const QString flagName = QStringLiteral("cutter_test_flag");
    const QString renamedFlag = QStringLiteral("cutter_test_renamed");
};

void TestFlagDialog::initTestCase()
{
    CUTTER_REQUIRE_BIN(const QString path, CutterTest::Bins::helloX64());
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
 * Regression: the size field used to be empty, and a new size wasn't applied.
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
 * Regression: a renamed flag used to keep ';' and '@', which rizin commands split on.
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
