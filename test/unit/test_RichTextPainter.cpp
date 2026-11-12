/**
 * @file test_RichTextPainter.cpp
 * @brief Cropping and converting the rich text drawn in the graph blocks.
 */

#include "IsolatedSettings.h"

#include "RichTextPainter.h"

#include <QTest>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>

class TestRichTextPainter : public QObject
{
    Q_OBJECT

private slots:
    void testCroppedUnchanged();
    void testCroppedTruncated();
    void testCroppedWithIndicator();
    void testCroppedMultiTokenIndicator();
    void testFromTextDocument();
};

/**
 * @brief Text shorter than the limit is kept and not reported as cropped.
 */
void TestRichTextPainter::testCroppedUnchanged()
{
    RichTextPainter::CustomRichText item;
    item.text = "hello";

    bool croppedOut = true;
    auto result = RichTextPainter::cropped({ item }, 10, "...", &croppedOut);

    QCOMPARE(result.size(), 1);
    QCOMPARE(result[0].text, QString("hello"));
    QVERIFY(!croppedOut);
}

/**
 * @brief Text longer than the limit is cut at the limit.
 */
void TestRichTextPainter::testCroppedTruncated()
{
    RichTextPainter::CustomRichText item;
    item.text = "0123456789";

    bool croppedOut = false;
    auto result = RichTextPainter::cropped({ item }, 5, "", &croppedOut);

    QCOMPARE(result.size(), 1);
    QCOMPARE(result[0].text, QString("01234"));
    QVERIFY(croppedOut);
}

/**
 * @brief A cropped text ends with the indicator, within the limit.
 */
void TestRichTextPainter::testCroppedWithIndicator()
{
    RichTextPainter::CustomRichText item;
    item.text = "abcdefghij"; // 10 chars

    bool croppedOut = false;
    // max 7 chars, indicator "..." (3 chars) -> "abcd..."
    auto result = RichTextPainter::cropped({ item }, 7, "...", &croppedOut);

    QCOMPARE(result.size(), 1);
    QCOMPARE(result[0].text, QString("abcd..."));
    QVERIFY(croppedOut);
}

/**
 * @brief Cropping across several tokens keeps the earlier ones and adds the indicator.
 */
void TestRichTextPainter::testCroppedMultiTokenIndicator()
{
    RichTextPainter::CustomRichText t1;
    t1.text = "abc"; // 3 chars
    RichTextPainter::CustomRichText t2;
    t2.text = "defghij"; // 7 chars

    bool croppedOut = false;
    // max 5 chars, total 10 chars. Truncates across tokens and inserts indicator
    auto result = RichTextPainter::cropped({ t1, t2 }, 5, "...", &croppedOut);

    QVERIFY(croppedOut);
    QString total;
    for (const auto &item : result) {
        total += item.text;
    }
    QCOMPARE(total, QString("ab..."));
}

/**
 * @brief Text and colors of a document become rich text items.
 */
void TestRichTextPainter::testFromTextDocument()
{
    QTextDocument doc;
    QTextCursor cursor(&doc);

    QTextCharFormat fmt;
    fmt.setForeground(QBrush(Qt::red));
    cursor.insertText("RedText", fmt);

    auto list = RichTextPainter::fromTextDocument(doc);

    QCOMPARE(list.size(), 1);
    QCOMPARE(list[0].text, QString("RedText"));
    QCOMPARE(list[0].flags, RichTextPainter::FlagColor);
    QCOMPARE(list[0].textColor, QColor(Qt::red));
}

CUTTER_UNIT_TEST_MAIN(TestRichTextPainter)
#include "test_RichTextPainter.moc"
