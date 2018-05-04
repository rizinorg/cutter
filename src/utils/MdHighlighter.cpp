#include <QtGui>

#include "utils/MdHighlighter.h"

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
        rule.pattern = QRegExp(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    singleLineCommentFormat.setFontWeight(QFont::Bold);
    singleLineCommentFormat.setForeground(Qt::darkGreen);
    rule.pattern = QRegExp(";[^\n]*");
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);
}

void MdHighlighter::highlightBlock(const QString &text)
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
}
