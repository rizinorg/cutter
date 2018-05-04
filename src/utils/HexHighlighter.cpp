#include <QtGui>

#include "utils/HexHighlighter.h"

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
        rule.pattern = QRegExp(pattern);
        rule.pattern.setCaseSensitivity(Qt::CaseInsensitive);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    singleLineCommentFormat.setFontWeight(QFont::Bold);
    singleLineCommentFormat.setForeground(Qt::darkGreen);
    rule.pattern = QRegExp(";[^\n]*");
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);

    commentStartExpression = QRegExp("/\\*");
    commentEndExpression = QRegExp("\\*/");
}

void HexHighlighter::highlightBlock(const QString &text)
{
    for (const HighlightingRule &rule : highlightingRules) {
        QRegExp expression(rule.pattern);
        int index = expression.indexIn(text);
        while (index >= 0) {
            int length = expression.matchedLength();
            setFormat(index, length, rule.format);
            index = expression.indexIn(text, index + length);
        }
    }
    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = commentStartExpression.indexIn(text);

    while (startIndex >= 0) {
        int endIndex = commentEndExpression.indexIn(text, startIndex);
        int commentLength;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex
                            + commentEndExpression.matchedLength();
        }
        setFormat(startIndex, commentLength, multiLineCommentFormat);
        startIndex = commentStartExpression.indexIn(text, startIndex + commentLength);
    }
}

