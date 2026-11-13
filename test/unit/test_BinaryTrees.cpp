/**
 * @file test_BinaryTrees.cpp
 * @brief Segment trees used by the graph grid layout.
 */

#include "IsolatedSettings.h"

#include "common/BinaryTrees.h"

#include <QTest>

class TestBinaryTrees : public QObject
{
    Q_OBJECT

private slots:
    void testPointSetMinTreeBasic();
    void testPointSetMinTreeLessThanQueries();
    void testRangeAssignMaxTree();
    void testMinMaxAccumulateTree();
};

/**
 * @brief Point updates of a min tree are read back by valueAtPoint.
 */
void TestBinaryTrees::testPointSetMinTreeBasic()
{
    // Initialize tree with 8 leaves and initial value 100
    PointSetMinTree tree(8, 100);

    for (size_t i = 0; i < 8; ++i) {
        QCOMPARE(tree.valueAtPoint(i), 100);
    }

    // Set point values
    tree.set(3, 15);
    tree.set(5, 42);

    QCOMPARE(tree.valueAtPoint(3), 15);
    QCOMPARE(tree.valueAtPoint(5), 42);
    QCOMPARE(tree.valueAtPoint(0), 100);
}

/**
 * @brief rightMostLessThan and leftMostLessThan find the nearest index below a value, or -1.
 */
void TestBinaryTrees::testPointSetMinTreeLessThanQueries()
{
    PointSetMinTree tree(8, 100);
    const std::vector<int> values = { 10, 20, 5, 15, 30, 2, 25, 40 };
    for (size_t i = 0; i < values.size(); ++i) {
        tree.set(i, values[i]);
    }

    QCOMPARE(tree.rightMostLessThan(4, 15), 2);

    QCOMPARE(tree.rightMostLessThan(7, 10), 5);

    QCOMPARE(tree.rightMostLessThan(1, 5), -1);

    QCOMPARE(tree.leftMostLessThan(0, 15), 0);

    QCOMPARE(tree.leftMostLessThan(3, 10), 5);

    QCOMPARE(tree.leftMostLessThan(6, 10), -1);
}

/**
 * @brief Range assignments of a max tree, including overlapping ones, give the right range maximum.
 */
void TestBinaryTrees::testRangeAssignMaxTree()
{
    RangeAssignMaxTree tree(10, 0);

    QCOMPARE(tree.rangeMaximum(0, 10), 0);

    tree.setRange(2, 6, 10);
    QCOMPARE(tree.rangeMaximum(0, 2), 0);
    QCOMPARE(tree.rangeMaximum(2, 6), 10);
    QCOMPARE(tree.rangeMaximum(0, 10), 10);

    tree.setRange(4, 8, 20);
    QCOMPARE(tree.rangeMaximum(2, 4), 10);
    QCOMPARE(tree.rangeMaximum(4, 8), 20);
    QCOMPARE(tree.rangeMaximum(2, 8), 20);
    QCOMPARE(tree.rangeMaximum(8, 10), 0);
}

/**
 * @brief Range updates of a min/max accumulate tree give the right range minimum and maximum.
 */
void TestBinaryTrees::testMinMaxAccumulateTree()
{
    // Test example case provided in class doc
    MinMaxAccumulateTree<int> tree(30);

    tree.updateRange(1, 5, 10);
    auto res1 = tree.rangeMinMax(0, 20);
    QCOMPARE(res1.first, 10);
    QCOMPARE(res1.second, 10);

    tree.updateRange(4, 6, 15);
    tree.updateRange(3, 10, 20);

    auto res2 = tree.rangeMinMax(0, 20);
    QCOMPARE(res2.first, 10);
    QCOMPARE(res2.second, 20);

    auto res3 = tree.rangeMinMax(1, 3);
    QCOMPARE(res3.first, 10);
    QCOMPARE(res3.second, 10);

    auto res4 = tree.rangeMinMax(5, 8);
    QCOMPARE(res4.first, 15);
    QCOMPARE(res4.second, 20);
}

CUTTER_UNIT_TEST_MAIN(TestBinaryTrees)
#include "test_BinaryTrees.moc"
