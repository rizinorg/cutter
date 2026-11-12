/**
 * @file test_HexdumpRangeDialog.cpp
 * @brief Choosing an address range by end address or by length.
 */

#include "TestEnv.h"
#include "UiHelpers.h"

#include "dialogs/HexdumpRangeDialog.h"

#include <QRadioButton>
#include <QTest>

namespace {

/**
 * @brief Types text into the named field as a user would, so its validation runs.
 */
bool typeInto(QWidget *dialog, const char *objectName, const QString &text)
{
    auto *edit = dialog->findChild<QLineEdit *>(objectName);
    if (!edit) {
        return false;
    }
    edit->clear();
    QTest::keyClicks(edit, text);
    return true;
}

QString fieldText(QWidget *dialog, const char *objectName)
{
    auto *edit = dialog->findChild<QLineEdit *>(objectName);
    return edit ? edit->text() : QString();
}

bool okEnabled(QWidget *dialog)
{
    auto *buttons = dialog->findChild<QDialogButtonBox *>();
    QPushButton *ok = buttons ? buttons->button(QDialogButtonBox::Ok) : nullptr;
    return ok && ok->isEnabled();
}

/**
 * @brief Switches the dialog to entering a length instead of an end.
 */
bool useLengthMode(QWidget *dialog)
{
    auto *radio = dialog->findChild<QRadioButton *>("lengthRadioButton");
    if (!radio) {
        return false;
    }
    radio->click();
    return radio->isChecked();
}

} // namespace

class TestHexdumpRangeDialog : public QObject
{
    Q_OBJECT

private slots:
    void endModeComputesLengthAndLastAddress();
    void lengthModeComputesEnd();
    void endBeforeStartIsInvalid();
    void emptyRangeNeedsAllowEmpty();
    void lengthOverflowIsInvalid();
    /**
     * @brief Reopening the dialog at a new address and pressing OK uses that address.
     *
     * HexWidget reuses one dialog. openAt() only sets the start field's text, so the start
     * from the previous use is kept and OK selects the old range. Red until fixed.
     */
    void okRightAfterOpenUsesShownStart();
};

/**
 * @brief An exclusive end gives the length and the inclusive last address.
 */
void TestHexdumpRangeDialog::endModeComputesLengthAndLastAddress()
{
    HexdumpRangeDialog dialog;
    dialog.show();
    QVERIFY(typeInto(&dialog, "startAddressLineEdit", "0x1000"));
    QVERIFY(typeInto(&dialog, "endAddressLineEdit", "0x1010"));

    QVERIFY(okEnabled(&dialog));
    QCOMPARE(fieldText(&dialog, "lengthLineEdit"), QString("0x10"));
    QCOMPARE(dialog.getStartAddress(), RVA(0x1000));
    QCOMPARE(dialog.getEndAddress(), RVA(0x100f));
    QVERIFY(!dialog.empty());
}

/**
 * @brief A length gives the end address.
 */
void TestHexdumpRangeDialog::lengthModeComputesEnd()
{
    HexdumpRangeDialog dialog;
    dialog.show();
    QVERIFY(useLengthMode(&dialog));
    QVERIFY(typeInto(&dialog, "startAddressLineEdit", "0x1000"));
    QVERIFY(typeInto(&dialog, "lengthLineEdit", "0x10"));

    QVERIFY(okEnabled(&dialog));
    QCOMPARE(fieldText(&dialog, "endAddressLineEdit"), QString("0x1010"));
    QCOMPARE(dialog.getEndAddress(), RVA(0x100f));
    QVERIFY(!dialog.empty());
}

/**
 * @brief An end before the start disables OK.
 */
void TestHexdumpRangeDialog::endBeforeStartIsInvalid()
{
    HexdumpRangeDialog dialog;
    dialog.show();
    QVERIFY(typeInto(&dialog, "startAddressLineEdit", "0x1010"));
    QVERIFY(typeInto(&dialog, "endAddressLineEdit", "0x1000"));

    QVERIFY(!okEnabled(&dialog));
    QCOMPARE(fieldText(&dialog, "lengthLineEdit"), QString("Invalid"));
}

/**
 * @brief An empty range enables OK only when the dialog allows it.
 */
void TestHexdumpRangeDialog::emptyRangeNeedsAllowEmpty()
{
    HexdumpRangeDialog strict(nullptr, false);
    strict.show();
    QVERIFY(typeInto(&strict, "startAddressLineEdit", "0x1000"));
    QVERIFY(typeInto(&strict, "endAddressLineEdit", "0x1000"));
    QVERIFY(!okEnabled(&strict));

    HexdumpRangeDialog lenient(nullptr, true);
    lenient.show();
    QVERIFY(typeInto(&lenient, "startAddressLineEdit", "0x1000"));
    QVERIFY(typeInto(&lenient, "endAddressLineEdit", "0x1000"));
    QVERIFY(okEnabled(&lenient));
    QVERIFY(lenient.empty());
}

/**
 * @brief A length that runs past the last address disables OK.
 */
void TestHexdumpRangeDialog::lengthOverflowIsInvalid()
{
    HexdumpRangeDialog dialog;
    dialog.show();
    QVERIFY(useLengthMode(&dialog));
    QVERIFY(typeInto(&dialog, "startAddressLineEdit", "0xffffffffffffff00"));
    QVERIFY(typeInto(&dialog, "lengthLineEdit", "0x200"));

    QVERIFY(!okEnabled(&dialog));
    QCOMPARE(fieldText(&dialog, "endAddressLineEdit"), QString("Invalid"));
}

/**
 * @brief OK right after opening uses the start address the dialog shows.
 *
 * To reproduce:
 * 1. In the Hexdump, right click → Select range, enter an end and press OK.
 * 2. Move the cursor, open Select range again and press OK straight away: the range started at the
 *    old address.
 */
void TestHexdumpRangeDialog::okRightAfterOpenUsesShownStart()
{
    HexdumpRangeDialog dialog;
    dialog.openAt(0x1000);
    QVERIFY(typeInto(&dialog, "endAddressLineEdit", "0x1010"));
    QCOMPARE(dialog.getStartAddress(), RVA(0x1000));

    dialog.openAt(0x2000);
    QCOMPARE(fieldText(&dialog, "startAddressLineEdit"), QString("0x2000"));
    QVERIFY2(!okEnabled(&dialog) || dialog.getStartAddress() == RVA(0x2000),
             "OK uses a start address other than the one shown");
}

CUTTER_CORE_TEST_MAIN(TestHexdumpRangeDialog)
#include "test_HexdumpRangeDialog.moc"
