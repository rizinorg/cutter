/**
 * @file test_BasicInstructionHighlighter.cpp
 * @brief Highlighted instruction ranges: lookup, partial clear and full clear.
 */

#include "IsolatedSettings.h"

#include "common/BasicInstructionHighlighter.h"

#include <QTest>

class TestInstructionHighlighters : public QObject
{
    Q_OBJECT

private slots:
    void testBasicLookup();
    void testPartialClearSplitsInstruction();
    void testFullOverlapClear();
};

/**
 * @brief An address inside a highlighted range finds it; the end address doesn't.
 */
void TestInstructionHighlighters::testBasicLookup()
{
    BasicInstructionHighlighter highlighter;

    highlighter.highlight(0x1000, 10, Qt::red);

    const auto *biStart = highlighter.getBasicInstruction(0x1000);
    QVERIFY(biStart != nullptr);
    QCOMPARE(biStart->address, RVA(0x1000));
    QCOMPARE(biStart->size, RVA(10));

    const auto *biMid = highlighter.getBasicInstruction(0x1005);
    QVERIFY(biMid != nullptr);

    const auto *biEnd = highlighter.getBasicInstruction(0x100A);
    QVERIFY(biEnd == nullptr);
}

/**
 * @brief Clearing the middle of a range leaves the parts before and after it highlighted.
 */
void TestInstructionHighlighters::testPartialClearSplitsInstruction()
{
    BasicInstructionHighlighter highlighter;
    highlighter.highlight(0x1000, 0x20, Qt::green);
    highlighter.clear(0x1008, 0x08);

    const auto *leftSegment = highlighter.getBasicInstruction(0x1004);
    QVERIFY(leftSegment != nullptr);
    QCOMPARE(leftSegment->address, RVA(0x1000));
    QCOMPARE(leftSegment->size, RVA(0x08));

    QVERIFY(highlighter.getBasicInstruction(0x1008) == nullptr);
    QVERIFY(highlighter.getBasicInstruction(0x100F) == nullptr);

    const auto *rightSegment = highlighter.getBasicInstruction(0x1010);
    QVERIFY(rightSegment != nullptr);
    QCOMPARE(rightSegment->address, RVA(0x1010));
    QCOMPARE(rightSegment->size, RVA(0x10));
}

/**
 * @brief Clearing a range that covers several highlights removes all of them.
 */
void TestInstructionHighlighters::testFullOverlapClear()
{
    BasicInstructionHighlighter highlighter;

    highlighter.highlight(0x1000, 0x05, Qt::blue);
    highlighter.highlight(0x1010, 0x05, Qt::yellow);

    highlighter.clear(0x0F00, 0x1100);

    QVERIFY(highlighter.getBasicInstruction(0x1000) == nullptr);
    QVERIFY(highlighter.getBasicInstruction(0x1010) == nullptr);
}

CUTTER_UNIT_TEST_MAIN(TestInstructionHighlighters)
#include "test_BasicInstructionHighlighter.moc"
