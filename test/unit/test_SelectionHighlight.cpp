/**
 * @file test_SelectionHighlight.cpp
 * @brief Highlighting the word under the cursor, matching braces and the current line.
 */

#include "IsolatedSettings.h"

#include "SelectionHighlight.h"

#include <QPlainTextEdit>
#include <QTest>
#include <QTextCursor>
#include <QTextDocument>

class TestSelectionHighlight : public QObject
{
    Q_OBJECT

private slots:
    void testEmptyWord();
    void testSameWordHighlight();
    void testBraceMatchingForward();
    void testBraceMatchingBackward();
    void testLineHighlights();
};

/**
 * @brief An empty word highlights nothing.
 */
void TestSelectionHighlight::testEmptyWord()
{
    QPlainTextEdit edit("int main() { return 0; }");
    auto selections = createSameWordsSelections(&edit, "");
    QVERIFY(selections.isEmpty());
}

/**
 * @brief Every whole occurrence of the word is highlighted, not parts of other words.
 */
void TestSelectionHighlight::testSameWordHighlight()
{
    QPlainTextEdit edit("int var = 1;\nint domain = 2;\nvar = var + 1;");
    auto selections = createSameWordsSelections(&edit, "var");
    QCOMPARE(selections.size(), 3);
}

/**
 * @brief An opening brace highlights its matching closing brace.
 */
void TestSelectionHighlight::testBraceMatchingForward()
{
    QPlainTextEdit edit("void foo() {\n    if (x) {\n        return;\n    }\n}");

    QTextCursor cursor = edit.document()->find("{", 0);
    QVERIFY(!cursor.isNull());
    cursor.setPosition(cursor.selectionStart());
    edit.setTextCursor(cursor);

    auto selections = createSameWordsSelections(&edit, "{");

    QCOMPARE(selections.size(), 2);
    QCOMPARE(selections[1].cursor.selectedText(), QString("}"));
}

/**
 * @brief A closing brace highlights its matching opening brace.
 */
void TestSelectionHighlight::testBraceMatchingBackward()
{
    QPlainTextEdit edit("void foo() {\n    if (x) {\n        return;\n    }\n}");

    QTextCursor cursor = edit.document()->find("}", edit.document()->characterCount(),
                                               QTextDocument::FindBackward);
    QVERIFY(!cursor.isNull());
    cursor.setPosition(cursor.selectionStart());
    edit.setTextCursor(cursor);

    auto selections = createSameWordsSelections(&edit, "}");

    QCOMPARE(selections.size(), 2);
    QCOMPARE(selections[1].cursor.selectedText(), QString("{"));
}

/**
 * @brief The current line and program counter highlights span the full width.
 */
void TestSelectionHighlight::testLineHighlights()
{
    QPlainTextEdit edit("Line 1\nLine 2");
    QTextCursor cursor = edit.textCursor();

    auto lineSel = createLineHighlightSelection(cursor);
    QVERIFY(lineSel.format.property(QTextFormat::FullWidthSelection).toBool());

    auto pcSel = createLineHighlightPC(cursor);
    QVERIFY(pcSel.format.property(QTextFormat::FullWidthSelection).toBool());

    auto bpSel = createLineHighlightBP(cursor);
    QVERIFY(bpSel.format.property(QTextFormat::FullWidthSelection).toBool());
}

CUTTER_UNIT_TEST_MAIN(TestSelectionHighlight)
#include "test_SelectionHighlight.moc"