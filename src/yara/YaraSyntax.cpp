
#include "YaraSyntax.h"

YaraSyntax::YaraSyntax(QTextDocument *parent)
    : QSyntaxHighlighter(parent), commentStartExpression("/\\*"), commentEndExpression("\\*/")
{
    HighlightingRule rule;

    // yara variables names (first to be done.)
    rule.pattern.setPattern("\\$?\\b[A-Za-z]([A-Za-z0-9_]+)?\\b[^\n]");
    rule.format.clearBackground();
    rule.format.clearForeground();
    rule.format.setFontWeight(QFont::Normal);
    rule.format.setForeground(QColor(175, 122, 197));
    highlightingRules.append(rule);

    rule.pattern.setPattern("\\brule\\b");
    rule.format.clearBackground();
    rule.format.clearForeground();
    rule.format.setFontWeight(QFont::Bold);
    rule.format.setForeground(Qt::red);
    highlightingRules.append(rule);

    rule.pattern.setPattern("\\ball of them\\b");
    rule.format.clearBackground();
    rule.format.clearForeground();
    rule.format.setFontWeight(QFont::Normal);
    rule.format.setForeground(QColor(80, 200, 215));
    highlightingRules.append(rule);

    // number/float
    rule.pattern.setPattern("\\b\\d+(\\.\\d+)?$");
    rule.format.clearBackground();
    rule.format.clearForeground();
    rule.format.setFontWeight(QFont::Normal);
    rule.format.setForeground(QColor(52, 152, 219));
    highlightingRules.append(rule);

    // true/false
    rule.pattern.setPattern("\\btrue\\b$");
    rule.format.clearBackground();
    rule.format.clearForeground();
    rule.format.setFontWeight(QFont::Normal);
    rule.format.setForeground(QColor(52, 152, 219));
    highlightingRules.append(rule);

    rule.pattern.setPattern("\\bfalse\\b$");
    rule.format.clearBackground();
    rule.format.clearForeground();
    rule.format.setFontWeight(QFont::Normal);
    rule.format.setForeground(QColor(52, 152, 219));
    highlightingRules.append(rule);

    // yara rule section
    rule.pattern.setPattern("\\b(meta|strings|condition)\\b:$");
    rule.format.clearBackground();
    rule.format.clearForeground();
    rule.format.setFontWeight(QFont::Normal);
    rule.format.setFontItalic(true);
    rule.format.setForeground(QColor(199, 0, 57));
    highlightingRules.append(rule);

    // bytes
    rule.pattern.setPattern(
            "(\\b[A-F0-9][A-F0-9]\\b|\\ \\?[A-F0-9]\\b|\\b[A-F0-9]\\?\\ |\\?\\?\\ )");
    rule.format.clearBackground();
    rule.format.clearForeground();
    rule.format.setFontWeight(QFont::Normal);
    rule.format.setFontItalic(false);
    rule.format.setForeground(QColor(241, 196, 15));
    highlightingRules.append(rule);

    // single-line comment
    rule.pattern.setPattern("//[^\n]*");
    rule.format.clearBackground();
    rule.format.clearForeground();
    rule.format.setFontWeight(QFont::Normal);
    rule.format.setFontItalic(false);
    rule.format.setForeground(Qt::gray);
    highlightingRules.append(rule);

    // quotation
    rule.pattern.setPattern("\".*\"");
    rule.format.clearBackground();
    rule.format.clearForeground();
    rule.format.setFontWeight(QFont::Normal);
    rule.format.setFontItalic(false);
    rule.format.setForeground(QColor(46, 204, 113));
    highlightingRules.append(rule);

    multiLineCommentFormat.setForeground(Qt::gray);
}

void YaraSyntax::highlightBlock(const QString &text)
{
    for (const auto &it : highlightingRules) {
        auto matchIterator = it.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            const auto match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), it.format);
        }
    }

    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1) {
        startIndex = text.indexOf(commentStartExpression);
    }

    while (startIndex >= 0) {
        const auto match = commentEndExpression.match(text, startIndex);
        const int endIndex = match.capturedStart();
        int commentLength = 0;

        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + match.capturedLength();
        }

        setFormat(startIndex, commentLength, multiLineCommentFormat);
        startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
    }
}
