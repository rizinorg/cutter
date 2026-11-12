/**
 * @file test_LinkedListPool.cpp
 * @brief The linked list pool used by the graph grid layout.
 */

#include "IsolatedSettings.h"

#include "common/LinkedListPool.h"

#include <QTest>
#include <vector>

class TestLinkedListPool : public QObject
{
    Q_OBJECT

private slots:
    void testMakeListAndIterate();
    void testAppend();
    void testAppendWithEmpty();
    void testSplitTail();
    void testSplitHead();
};

/**
 * @brief A one-item list iterates over its item; a default list is empty.
 */
void TestLinkedListPool::testMakeListAndIterate()
{
    LinkedListPool<int> pool(10);
    const LinkedListPool<int>::List emptyList;
    QVERIFY(emptyList.isEmpty());

    auto list = pool.makeList(42);
    QVERIFY(!list.isEmpty());

    auto it = pool.head(list);
    QVERIFY(static_cast<bool>(it));
    QCOMPARE(*it, 42);

    ++it;
    QVERIFY(!static_cast<bool>(it));
}

/**
 * @brief Appending lists keeps their items in order.
 */
void TestLinkedListPool::testAppend()
{
    LinkedListPool<int> pool(10);
    auto l1 = pool.makeList(10);
    auto l2 = pool.makeList(20);
    auto l3 = pool.makeList(30);

    auto combined = pool.append(l1, l2);
    combined = pool.append(combined, l3);

    std::vector<int> values;
    for (auto it = pool.head(combined); it; ++it) {
        values.push_back(*it);
    }

    const std::vector<int> expected = { 10, 20, 30 };
    QCOMPARE(values, expected);
}

/**
 * @brief Appending an empty list on either side gives the other list.
 */
void TestLinkedListPool::testAppendWithEmpty()
{
    LinkedListPool<int> pool(10);
    LinkedListPool<int>::List emptyList;
    auto validList = pool.makeList(100);

    auto res1 = pool.append(emptyList, validList);
    QCOMPARE(*pool.head(res1), 100);

    auto res2 = pool.append(validList, emptyList);
    QCOMPARE(*pool.head(res2), 100);
}

/**
 * @brief splitTail returns the items from the iterator to the end.
 */
void TestLinkedListPool::testSplitTail()
{
    LinkedListPool<int> pool(10);
    auto l1 = pool.makeList(100);
    auto l2 = pool.makeList(200);
    auto l3 = pool.makeList(300);

    auto combined = pool.append(pool.append(l1, l2), l3); // [100, 200, 300]

    auto it = pool.head(combined);
    ++it; // points to 200

    auto tailPart = pool.splitTail(combined, it);

    std::vector<int> tailValues;
    for (auto itTail = pool.head(tailPart); itTail; ++itTail) {
        tailValues.push_back(*itTail);
    }

    const std::vector<int> expectedTail = { 200, 300 };
    QCOMPARE(tailValues, expectedTail);
}

/**
 * @brief splitHead returns the items before the iterator.
 */
void TestLinkedListPool::testSplitHead()
{
    LinkedListPool<int> pool(10);
    auto l1 = pool.makeList(100);
    auto l2 = pool.makeList(200);
    auto l3 = pool.makeList(300);

    auto combined = pool.append(pool.append(l1, l2), l3); // [100, 200, 300]

    auto it = pool.head(combined);
    ++it;
    ++it; // Points to 300

    auto headPart = pool.splitHead(combined, it);

    std::vector<int> headValues;
    for (auto itHead = pool.head(headPart); itHead; ++itHead) {
        headValues.push_back(*itHead);
    }

    const std::vector<int> expectedHead = { 100, 200 };
    QCOMPARE(headValues, expectedHead);
}

CUTTER_UNIT_TEST_MAIN(TestLinkedListPool)
#include "test_LinkedListPool.moc"
