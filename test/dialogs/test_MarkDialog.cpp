/**
 * @file test_MarkDialog.cpp
 * @brief Adding and renaming marks through the mark dialog.
 */

#include "Fixtures.h"
#include "TestEnv.h"
#include "UiHelpers.h"

#include "dialogs/MarkDialog.h"

#include <QTest>

namespace {

constexpr RVA markLength = 0x10;

const MarkDescription *findMark(const QList<MarkDescription> &marks, const QString &name)
{
    for (const MarkDescription &mark : marks) {
        if (mark.name == name) {
            return &mark;
        }
    }
    return nullptr;
}

} // namespace

class TestMarkDialog : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();

    /**
     * @brief Renaming a mark to another mark's name is refused and both marks stay.
     */
    void renameOntoTakenNameIsRejected();
    /**
     * @brief Adding a mark under a taken name is refused and the other mark stays.
     */
    void addWithTakenNameIsRejected();

private:
    void verifyOtherMarkUnchanged();

    RVA from = RVA_INVALID;
    const QString name = QStringLiteral("cutter_test_mark");
    const QString otherName = QStringLiteral("cutter_test_other");
    const QString otherComment = QStringLiteral("other comment");
};

void TestMarkDialog::initTestCase()
{
    CUTTER_REQUIRE_FIXTURE(const QString path, CutterTest::Fixtures::helloX64());
    QVERIFY(CutterTest::openFile(path));
    from = CutterTest::core()->getOffset();
    QVERIFY(from != RVA_INVALID);
}

void TestMarkDialog::init()
{
    CutterCore *core = CutterTest::core();
    const QList<MarkDescription> marks = core->getMarks();
    for (const MarkDescription &mark : marks) {
        core->delMark(mark.name);
    }
    core->addMark(from + 2 * markLength, from + 3 * markLength, otherName, otherComment,
                  Qt::red);
}

void TestMarkDialog::verifyOtherMarkUnchanged()
{
    const QList<MarkDescription> marks = CutterTest::core()->getMarks();
    const MarkDescription *other = findMark(marks, otherName);
    QVERIFY(other);
    QCOMPARE(other->from, from + 2 * markLength);
    QCOMPARE(other->to, from + 3 * markLength);
    QCOMPARE(other->comment, otherComment);
}

/**
 * @brief Renaming a mark to another mark's name is refused and the other mark is unchanged.
 *
 * To reproduce:
 * 1. Add two marks.
 * 2. Edit one and give it the other's name: the other mark's range, comment and color were
 *    replaced.
 */
void TestMarkDialog::renameOntoTakenNameIsRejected()
{
    CutterTest::core()->addMark(from, from + markLength, name, QString(), Qt::black);
    CutterTest::MessageBoxWatcher warnings;

    MarkDialog dialog(from, from + markLength, nullptr, name);
    QVERIFY(CutterTest::setLineEditText(&dialog, "nameEdit", otherName));
    QVERIFY(CutterTest::clickStandardButton(&dialog, QDialogButtonBox::Ok));

    QCOMPARE(warnings.seenTexts().size(), 1);
    QVERIFY(dialog.result() != QDialog::Accepted);
    QVERIFY(findMark(CutterTest::core()->getMarks(), name));
    verifyOtherMarkUnchanged();
}

/**
 * @brief Adding a mark with a name already in use is refused and the other mark is unchanged.
 *
 * To reproduce:
 * 1. Add a mark.
 * 2. Add another with the same name: the first mark's range, comment and color were replaced.
 */
void TestMarkDialog::addWithTakenNameIsRejected()
{
    CutterTest::MessageBoxWatcher warnings;

    MarkDialog dialog(from, from + markLength);
    QVERIFY(CutterTest::setLineEditText(&dialog, "nameEdit", otherName));
    QVERIFY(CutterTest::clickStandardButton(&dialog, QDialogButtonBox::Ok));

    QCOMPARE(warnings.seenTexts().size(), 1);
    QVERIFY(dialog.result() != QDialog::Accepted);
    verifyOtherMarkUnchanged();
}

CUTTER_CORE_TEST_MAIN(TestMarkDialog)
#include "test_MarkDialog.moc"
