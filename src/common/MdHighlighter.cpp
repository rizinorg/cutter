#include <QtGui>

#include "common/MdHighlighter.h"

MdHighlighter::MdHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    keywordFormat.setForeground(Qt::gray);
    keywordFormat.setFontWeight(QFont::Bold);

    QStringList keywordPatterns;
    keywordPatterns << "^\\#{1,6}[ A-Za-z]+\\b" << "\\*\\*([^\\\\]+)\\*\\*"
                    << "\\*([^\\\\]+)\\*" << "\\_([^\\\\]+)\\_"
                    << "\\_\\_([^\\\\]+)\\_\\_";

    for (const QString &pattern : keywordPatterns) {
        rule.pattern.setPattern(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    singleLineCommentFormat.setFontWeight(QFont::Bold);
    singleLineCommentFormat.setForeground(Qt::darkGreen);
    rule.pattern.setPattern(";[^\n]*");
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);
}

void MdHighlighter::highlightBlock(const QString &text)
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
}
