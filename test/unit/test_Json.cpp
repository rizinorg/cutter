/**
 * @file test_Json.cpp
 * @brief Showing rizin JSON as a tree of items.
 */

#include "IsolatedSettings.h"

#include "CutterJson.h"
#include "Json.h"

#include <QTreeWidgetItem>
#include <QtTest/QtTest>

#include <memory>

/**
 * @brief Parses JSON text the way Cutter reads rizin's JSON output.
 */
static CutterJson parseJson(const char *src)
{
    char *buf = rz_str_dup(src);
    RzJson *parsed = rz_json_parse(buf);
    auto owner = std::make_shared<CutterJsonOwner>(parsed, buf);
    return CutterJson(parsed, owner);
}

class TestJson : public QObject
{
    Q_OBJECT

private slots:
    void testJsonTreeWidgetItemPrimitives();
    void testJsonTreeWidgetItemObjectAndArray();
};

/**
 * @brief Strings, numbers, booleans and null become one item with their value.
 */
void TestJson::testJsonTreeWidgetItemPrimitives()
{
    // String
    {
        auto json = parseJson("\"hello_world\"");
        auto *item = Cutter::jsonTreeWidgetItem("strKey", json);
        QCOMPARE(item->text(0), QString("strKey"));
        QCOMPARE(item->text(1), QString("hello_world"));
        delete item;
    }

    // Boolean
    {
        auto json = parseJson("true");
        auto *item = Cutter::jsonTreeWidgetItem("boolKey", json);
        QCOMPARE(item->text(0), QString("boolKey"));
        QCOMPARE(item->text(1), QString("true"));
        delete item;
    }

    // Integer
    {
        auto json = parseJson("42");
        auto *item = Cutter::jsonTreeWidgetItem("intKey", json);
        QCOMPARE(item->text(0), QString("intKey"));
        QCOMPARE(item->text(1), QString("42"));
        delete item;
    }

    // Double
    {
        auto json = parseJson("3.14");
        auto *item = Cutter::jsonTreeWidgetItem("dblKey", json);
        QCOMPARE(item->text(0), QString("dblKey"));
        QCOMPARE(item->text(1), QString::number(3.14));
        delete item;
    }

    // Null
    {
        auto json = parseJson("null");
        auto *item = Cutter::jsonTreeWidgetItem("nullKey", json);
        QCOMPARE(item->text(0), QString("nullKey"));
        QCOMPARE(item->text(1), QString("null"));
        delete item;
    }
}

/**
 * @brief Objects and arrays become an item with one child per member.
 */
void TestJson::testJsonTreeWidgetItemObjectAndArray()
{
    // Object
    {
        auto json = parseJson("{\"keyA\": \"valA\", \"keyB\": 100}");
        auto *item = Cutter::jsonTreeWidgetItem("objRoot", json);
        QCOMPARE(item->text(0), QString("objRoot"));
        QCOMPARE(item->text(1), QString(""));
        QCOMPARE(item->childCount(), 2);

        QCOMPARE(item->child(0)->text(0), QString("keyA"));
        QCOMPARE(item->child(0)->text(1), QString("valA"));

        QCOMPARE(item->child(1)->text(0), QString("keyB"));
        QCOMPARE(item->child(1)->text(1), QString("100"));

        delete item;
    }

    // Array
    {
        auto json = parseJson("[\"first\", \"second\"]");
        auto *item = Cutter::jsonTreeWidgetItem("arrRoot", json);
        QCOMPARE(item->text(0), QString("arrRoot"));
        QCOMPARE(item->childCount(), 2);

        QCOMPARE(item->child(0)->text(0), QString("0"));
        QCOMPARE(item->child(0)->text(1), QString("first"));

        QCOMPARE(item->child(1)->text(0), QString("1"));
        QCOMPARE(item->child(1)->text(1), QString("second"));

        delete item;
    }
}

CUTTER_UNIT_TEST_MAIN(TestJson)
#include "test_Json.moc"