/**
 * @file test_DisassemblyHelper.cpp
 * @brief The word, bracket expression, offset and arrow under the cursor in disassembly text.
 */

#include "IsolatedSettings.h"

#include "DisassemblyHelper.h"

#include <QtTest/QtTest>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextCursor>

class TestDisassemblyHelper : public QObject
{
    Q_OBJECT

private slots:
    void testFindBracketRange_data();
    void testFindBracketRange();
    void testReadDisassemblyDataFromCursor();
    void testGetContextFromCursor();
};

void TestDisassemblyHelper::testFindBracketRange_data()
{
    QTest::addColumn<QString>("line");
    QTest::addColumn<int>("posInLine");
    QTest::addColumn<bool>("expectedFound");
    QTest::addColumn<int>("expectedStart");
    QTest::addColumn<int>("expectedLength");
    QTest::addColumn<QString>("expectedContent");

    QTest::newRow("inside_brackets")
        << "mov eax, [0x1000]" << 12 << true << 9 << 8 << "[0x1000]";

    QTest::newRow("at_open_bracket")
        << "mov eax, [0x1000]" << 9 << true << 9 << 8 << "[0x1000]";

    QTest::newRow("outside_brackets")
        << "mov eax, [0x1000]" << 3 << false << 0 << 0 << "";

    QTest::newRow("second_bracket_pair")
        << "lea rax, [rbx + [rcx]]" << 18 << true << 16 << 5 << "[rcx]";

    QTest::newRow("unmatched_bracket")
        << "mov eax, [0x1000" << 11 << false << 0 << 0 << "";
}

/**
 * @brief findBracketRange returns the bracket expression around a position, including on its
 *        opening '['.
 */
void TestDisassemblyHelper::testFindBracketRange()
{
    QFETCH(const QString, line);
    QFETCH(const int, posInLine);
    QFETCH(const bool, expectedFound);
    QFETCH(const int, expectedStart);
    QFETCH(const int, expectedLength);
    QFETCH(const QString, expectedContent);

    const auto res = DisassemblyHelper::findBracketRange(line, posInLine);

    QCOMPARE(res.found, expectedFound);
    if (expectedFound) {
        QCOMPARE(res.start, expectedStart);
        QCOMPARE(res.length, expectedLength);
        QCOMPARE(res.content, expectedContent);
    }
}

/**
 * @brief The offset and arrow stored in a line's block data are read back.
 */
void TestDisassemblyHelper::testReadDisassemblyDataFromCursor()
{
    QTextDocument doc;
    QTextCursor cursor(&doc);

    cursor.insertText("0x00401000: mov eax, 1\n");

    DisassemblyLine line;
    line.offset = 0x00401000;
    line.arrow = 0x00401050;

    auto *userData = new DisassemblyTextBlockUserData(line);
    cursor.block().setUserData(userData);

    QCOMPARE(DisassemblyHelper::readDisassemblyOffset(cursor), RVA(0x00401000));
    QCOMPARE(DisassemblyHelper::readDisassemblyArrow(cursor), RVA(0x00401050));
}

/**
 * @brief getContextFromCursor returns the bracket expression, line, offset and arrow under the
 *        cursor.
 */
void TestDisassemblyHelper::testGetContextFromCursor()
{
    QTextDocument doc;
    QTextCursor cursor(&doc);

    cursor.insertText("mov eax, [0x402000]");

    DisassemblyLine line;
    line.offset = 0x00401000;
    line.arrow = RVA_INVALID;

    cursor.block().setUserData(new DisassemblyTextBlockUserData(line));

    // place cursor over bracketed memory address: "[0x402000]"
    cursor.setPosition(12);

    const auto ctx = DisassemblyHelper::getContextFromCursor(cursor);

    QCOMPARE(ctx.word, QString("[0x402000]"));
    QCOMPARE(ctx.line, QString("mov eax, [0x402000]"));
    QCOMPARE(ctx.offset, RVA(0x00401000));
    QCOMPARE(ctx.arrow, RVA_INVALID);
}

CUTTER_UNIT_TEST_MAIN(TestDisassemblyHelper)
#include "test_DisassemblyHelper.moc"
