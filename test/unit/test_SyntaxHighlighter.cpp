/**
 * @file test_SyntaxHighlighter.cpp
 * @brief The fallback C highlighter, used when KSyntaxHighlighting is not available.
 */

#include "IsolatedSettings.h"

#include "common/SyntaxHighlighter.h"

#include <QTextBlock>
#include <QTextDocument>
#include <QTextLayout>
#include <QtTest/QtTest>

namespace {

class Highlighted
{
public:
    explicit Highlighted(const QString &text) : highlighter(&document)
    {
        document.setPlainText(text);
        highlighter.rehighlight();
    }

    QColor colorAt(int position) const
    {
        const QTextBlock block = document.findBlock(position);
        const int offset = position - block.position();
        for (const QTextLayout::FormatRange &range : block.layout()->formats()) {
            if (offset >= range.start && offset < range.start + range.length) {
                return range.format.foreground().color();
            }
        }
        return {};
    }

    const FallbackSyntaxHighlighter &rules() const { return highlighter; }

    bool spanHasColor(int start, int length, const QColor &color) const
    {
        for (int position = start; position < start + length; ++position) {
            if (colorAt(position) != color) {
                return false;
            }
        }
        return true;
    }

private:
    QTextDocument document;
    FallbackSyntaxHighlighter highlighter;
};

} // namespace

class TestSyntaxHighlighter : public QObject
{
    Q_OBJECT

private slots:
    void everyKeywordIsHighlighted_data();
    void everyKeywordIsHighlighted();
    void keywordInsideIdentifierIsNotHighlighted_data();
    void keywordInsideIdentifierIsNotHighlighted();
    void functionCallIsHighlighted();
    void lineCommentCoversRestOfLine();
    void stringLiteralIsHighlighted();
    void blockCommentSpansLinesAndEnds();
};

void TestSyntaxHighlighter::everyKeywordIsHighlighted_data()
{
    QTest::addColumn<QString>("keyword");
    const FallbackSyntaxHighlighter highlighter;
    for (const QString &keyword : highlighter.keywords()) {
        QTest::newRow(qPrintable(keyword)) << keyword;
    }
}

/**
 * @brief Every C keyword is highlighted as a keyword.
 */
void TestSyntaxHighlighter::everyKeywordIsHighlighted()
{
    QFETCH(const QString, keyword);

    const QString text = QString("x %1 y").arg(keyword);
    const int start = text.indexOf(keyword);
    const Highlighted highlighted(text);

    QVERIFY(highlighted.spanHasColor(start, keyword.size(),
                                     highlighted.rules().keywordColor()));
    QVERIFY(highlighted.colorAt(start - 1) != highlighted.rules().keywordColor());
    QVERIFY(highlighted.colorAt(start + keyword.size())
            != highlighted.rules().keywordColor());
}

void TestSyntaxHighlighter::keywordInsideIdentifierIsNotHighlighted_data()
{
    everyKeywordIsHighlighted_data();
}

/**
 * @brief A keyword inside a longer identifier isn't highlighted.
 */
void TestSyntaxHighlighter::keywordInsideIdentifierIsNotHighlighted()
{
    QFETCH(const QString, keyword);

    const QString text = QString("x_%1_y").arg(keyword);
    const Highlighted highlighted(text);

    for (int position = 0; position < text.size(); ++position) {
        QVERIFY(highlighted.colorAt(position) != highlighted.rules().keywordColor());
    }
}

/**
 * @brief A name followed by '(' is highlighted as a function.
 */
void TestSyntaxHighlighter::functionCallIsHighlighted()
{
    const QString name = "compute";
    const QString text = QString("x = %1(y);").arg(name);
    const int start = text.indexOf(name);
    const Highlighted highlighted(text);

    QVERIFY(highlighted.spanHasColor(start, name.size(),
                                     highlighted.rules().functionColor()));
    QVERIFY(highlighted.colorAt(text.indexOf('y'))
            != highlighted.rules().functionColor());
}

/**
 * @brief A // comment covers the rest of its line only.
 */
void TestSyntaxHighlighter::lineCommentCoversRestOfLine()
{
    const QString code = "x = y;";
    const QString text = code + " // note";
    const int start = text.indexOf("//");
    const Highlighted highlighted(text);

    QVERIFY(highlighted.spanHasColor(start, text.size() - start,
                                     highlighted.rules().commentColor()));
    QVERIFY(!highlighted.spanHasColor(0, code.size(), highlighted.rules().commentColor()));
}

/**
 * @brief A string literal is highlighted as a string.
 */
void TestSyntaxHighlighter::stringLiteralIsHighlighted()
{
    const QString literal = "\"hello world\"";
    const QString text = QString("s = %1;").arg(literal);
    const int start = text.indexOf(literal);
    const Highlighted highlighted(text);

    QVERIFY(highlighted.spanHasColor(start, literal.size(),
                                     highlighted.rules().stringColor()));
    QVERIFY(highlighted.colorAt(start - 1) != highlighted.rules().stringColor());
    QVERIFY(highlighted.colorAt(start + literal.size())
            != highlighted.rules().stringColor());
}

/**
 * @brief A block comment spans lines and ends at its closing marker.
 */
void TestSyntaxHighlighter::blockCommentSpansLinesAndEnds()
{
    const QString comment = "/* first\n middle\n last */";
    const QString text = comment + " y";
    const Highlighted highlighted(text);

    for (int position = 0; position < comment.size(); ++position) {
        if (comment.at(position) != '\n') {
            QVERIFY2(highlighted.colorAt(position) == highlighted.rules().commentColor(),
                     qPrintable(QString("position %1 is not a comment").arg(position)));
        }
    }
    QVERIFY(highlighted.colorAt(text.indexOf('y', comment.size()))
            != highlighted.rules().commentColor());
}

CUTTER_UNIT_TEST_MAIN(TestSyntaxHighlighter)
#include "test_SyntaxHighlighter.moc"
