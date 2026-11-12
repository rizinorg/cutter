/**
 * @file test_Helpers.cpp
 * @brief Small helpers from common/Helpers.h.
 */

#include "IsolatedSettings.h"

#include "common/Helpers.h"

#include <QTest>
#include <QSignalSpy>
#include <QPushButton>
#include <QComboBox>
#include <QTreeWidget>

class TestHelpers : public QObject
{
    Q_OBJECT

private slots:
    void testFormatByteCount();
    void testSetCheckedWithoutSignals();
    void testSelectIndexByData();
    void testApplyColorToSvg();
    void testAppendRow();
};

/**
 * @brief Byte counts are shown with the right unit.
 */
void TestHelpers::testFormatByteCount()
{
    QCOMPARE(qhelpers::formatByteCount(0), QString("0"));
    QCOMPARE(qhelpers::formatByteCount(512), QString("512  B")); // Two spaces for raw bytes
    QCOMPARE(qhelpers::formatByteCount(1024), QString("1 kB"));
    QCOMPARE(qhelpers::formatByteCount(1048576), QString("1 MB"));
    QCOMPARE(qhelpers::formatByteCount(1073741824), QString("1 GB"));
}

/**
 * @brief setCheckedWithoutSignals checks a button without emitting toggled.
 */
void TestHelpers::testSetCheckedWithoutSignals()
{
    QPushButton button("Test");
    button.setCheckable(true);

    QSignalSpy spy(&button, &QPushButton::toggled);

    // Verify signal is NOT emitted when using qhelpers::setCheckedWithoutSignals
    qhelpers::setCheckedWithoutSignals(&button, true);
    QCOMPARE(spy.count(), 0);
    QVERIFY(button.isChecked());
}

/**
 * @brief selectIndexByData selects the item with the data, or the default index.
 */
void TestHelpers::testSelectIndexByData()
{
    QComboBox combo;
    combo.addItem("Item 0", 100);
    combo.addItem("Item 1", 200);
    combo.addItem("Item 2", 300);

    // Select existing data
    qhelpers::selectIndexByData(&combo, 200, 0);
    QCOMPARE(combo.currentIndex(), 1);

    // Select non-existing data, should fallback to defaultIndex
    qhelpers::selectIndexByData(&combo, 999, 2);
    QCOMPARE(combo.currentIndex(), 2);
}

/**
 * @brief applyColorToSvg replaces every fill color of an SVG.
 */
void TestHelpers::testApplyColorToSvg()
{
    const QByteArray originalSvg = R"(<svg><path fill="#000000" style="fill:#123456;"/></svg>)";
    const QColor newColor(0xFF, 0x00, 0x00); // Red (#ff0000)

    const QByteArray coloredSvg = qhelpers::applyColorToSvg(originalSvg, newColor);

    QVERIFY(!coloredSvg.contains("#000000"));
    QVERIFY(!coloredSvg.contains("#123456"));
    QVERIFY(coloredSvg.contains("#ff0000"));
}


/**
 * @brief appendRow adds a top-level row with the texts after the hidden first column.
 */
void TestHelpers::testAppendRow()
{
    QTreeWidget treeWidget;
    treeWidget.setColumnCount(6);

    QTreeWidgetItem *item = qhelpers::appendRow(&treeWidget, "Col1", "Col2", "Col3");

    QVERIFY(item != nullptr);
    QCOMPARE(treeWidget.topLevelItemCount(), 1);
    QCOMPARE(item->text(0), QString("0")); // dummy hidden column
    QCOMPARE(item->text(1), QString("Col1"));
    QCOMPARE(item->text(2), QString("Col2"));
    QCOMPARE(item->text(3), QString("Col3"));
}


CUTTER_UNIT_TEST_MAIN(TestHelpers)
#include "test_Helpers.moc"
