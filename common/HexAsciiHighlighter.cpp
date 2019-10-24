#include <QtGui>

#include "common/HexAsciiHighlighter.h"

AsciiHighlighter::AsciiHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    asciiFormat.setForeground(QColor(65, 131, 215));
    rule.pattern.setPattern("\\b[A-Za-z0-9]+\\b");
    rule.format = asciiFormat;
    highlightingRules.append(rule);

    commentStartRegularExpression.setPattern("/\\*");
    commentEndRegularExpression.setPattern("\\*/");
}

void AsciiHighlighter::highlightBlock(const QString &text)
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
