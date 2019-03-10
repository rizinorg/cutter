#include "Highlighter.h"
#include "core/MainWindow.h"

#include <QtGui>

Highlighter::Highlighter(QTextDocument *parent) :
    QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    core = Core();

    keywordFormat.setForeground(QColor(65, 131, 215));

    for (const QString &pattern : this->core->opcodes) {
        rule.pattern = QRegExp("\\b" + pattern + "\\b");
        rule.pattern.setCaseSensitivity(Qt::CaseInsensitive);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    regFormat.setForeground(QColor(236, 100, 75));

    for (const QString &pattern : this->core->regs) {
        rule.pattern = QRegExp("\\b" + pattern + "\\b");
        rule.pattern.setCaseSensitivity(Qt::CaseInsensitive);
        rule.format = regFormat;
        highlightingRules.append(rule);
    }


    singleLineCommentFormat.setFontWeight(QFont::Bold);
    singleLineCommentFormat.setForeground(QColor(63, 195, 128));
    rule.pattern = QRegExp(";[^\n]*");
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);

    commentStartExpression = QRegExp("/\\*");
    commentEndExpression = QRegExp("\\*/");
}

void Highlighter::highlightBlock(const QString &text)
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
