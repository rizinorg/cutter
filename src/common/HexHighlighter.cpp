#include <QtGui>

#include "common/HexHighlighter.h"

HexHighlighter::HexHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    keywordFormat.setForeground(QColor(65, 131, 215));
    keywordFormat.setFontWeight(QFont::Bold);
    QStringList keywordPatterns;
    // I know, your eyes are bleeding, mine too
    keywordPatterns << "\\b20\\b" << "\\b21\\b" << "\\b22\\b" << "\\b23\\b" << "\\b24\\b" << "\\b25\\b"
                    << "\\b26\\b"
                    << "\\b27\\b" << "\\b28\\b" << "\\b29\\b" << "\\b2a\\b" << "\\b2b\\b" << "\\b2c\\b" << "\\b2d\\b"
                    << "\\b2e\\b" << "\\b2f\\b" << "\\b30\\b" << "\\b31\\b" << "\\b32\\b" << "\\b33\\b" << "\\b34\\b"
                    << "\\b35\\b" << "\\b36\\b" << "\\b37\\b" << "\\b38\\b" << "\\b39\\b" << "\\b3a\\b" << "\\b3b\\b"
                    << "\\b3c\\b" << "\\b3d\\b" << "\\b3e\\b" << "\\b3f\\b" << "\\b41\\b" << "\\b42\\b" << "\\b43\\b"
                    << "\\b44\\b" << "\\b45\\b" << "\\b46\\b" << "\\b47\\b" << "\\b48\\b" << "\\b49\\b" << "\\b4a\\b"
                    << "\\b4b\\b" << "\\b4c\\b" << "\\b4d\\b" << "\\b4e\\b" << "\\b4f\\b" << "\\b50\\b" << "\\b51\\b"
                    << "\\b52\\b" << "\\b53\\b" << "\\b54\\b" << "\\b55\\b" << "\\b56\\b" << "\\b57\\b" << "\\b58\\b"
                    << "\\b59\\b" << "\\b5a\\b" << "\\b5b\\b" << "\\b5c\\b" << "\\b5d\\b" << "\\b5e\\b" << "\\b5f\\b"
                    << "\\b60\\b" << "\\b61\\b" << "\\b62\\b" << "\\b63\\b" << "\\b64\\b" << "\\b65\\b" << "\\b66\\b"
                    << "\\b67\\b" << "\\b68\\b" << "\\b69\\b" << "\\b6a\\b" << "\\b6b\\b" << "\\b6c\\b" << "\\b6d\\b"
                    << "\\b6e\\b" << "\\b6f\\b" << "\\b70\\b" << "\\b71\\b" << "\\b72\\b" << "\\b73\\b" << "\\b74\\b"
                    << "\\b75\\b" << "\\b76\\b" << "\\b77\\b" << "\\b78\\b" << "\\b79\\b" << "\\b7a\\b" << "\\b7b\\b"
                    << "\\b7c\\b" << "\\b7d\\b" << "\\b7e\\b" << "\\b7f\\b";
    for (const QString &pattern : keywordPatterns) {
        rule.pattern.setPattern(pattern);
        rule.pattern.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    singleLineCommentFormat.setFontWeight(QFont::Bold);
    singleLineCommentFormat.setForeground(Qt::darkGreen);
    rule.pattern.setPattern(";[^\n]*");
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);

    commentStartRegularExpression.setPattern("/\\*");
    commentEndRegularExpression.setPattern("\\*/");
}

void HexHighlighter::highlightBlock(const QString &text)
{
    for (const HighlightingRule &rule : highlightingRules) {
        QRegularExpression expression(rule.pattern);
        int index = expression.match(text).capturedStart();
        while (index >= 0) {
            int length = expression.match(text).capturedLength();
            setFormat(index, length, rule.format);
            index = expression.match(text.mid(index + length)).capturedStart();
        }
    }
    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = QRegularExpression(commentStartRegularExpression).match(text).capturedStart();

    while (startIndex >= 0) {
        QRegularExpressionMatch commentEndMatch = QRegularExpression(commentEndRegularExpression).match(text.mid(startIndex));
        int endIndex = commentEndMatch.capturedStart();
        int commentLength;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex
                            + commentEndMatch.capturedLength();
        }
        setFormat(startIndex, commentLength, multiLineCommentFormat);
        startIndex = QRegularExpression(commentStartRegularExpression).match(text.mid(startIndex + commentLength)).capturedStart();
    }
}
