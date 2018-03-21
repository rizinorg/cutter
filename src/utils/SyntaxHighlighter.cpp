#include "SyntaxHighlighter.h"

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent)
    :   QSyntaxHighlighter(parent)
    ,   commentStartExpression("/\\*")
    ,   commentEndExpression("\\*/")
{
    HighlightingRule rule;
    QStringList keywordPatterns;

    //C language keywords
    keywordPatterns << "\\bauto\\b" << "\\bdouble\\b" << "\\bint\\b"
                    << "\\bstruct\\b" << "\\bbreak\\b" << "\\belse\\b"
                    << "\\blong\\b" << "\\switch\\b" << "\\bcase\\b"
                    << "\\benum\\b" << "\\bregister\\b" << "\\btypedef\\b"
                    << "\\bchar\\b" << "\\bextern\\b" << "\\breturn\\b"
                    << "\\bunion\\b" << "\\bconst\\b" << "\\bfloat\\b"
                    << "\\bshort\\b" << "\\bunsigned\\b" << "\\bcontinue\\b"
                    << "\\bfor\\b" << "\\bsigned\\b" << "\\bvoid\\b"
                    << "\\bdefault\\b" << "\\bgoto\\b" << "\\bsizeof\\b"
                    << "\\bvolatile\\b" << "\\bdo\\b" << "\\bif\\b"
                    << "\\static\\b" << "\\while\\b";
    //Special words
    keywordPatterns << "\\bloc_*\\b" << "\\bsym.*\\b";

    QTextCharFormat keywordFormat;
    keywordFormat.setForeground(Qt::red);
    keywordFormat.setFontWeight(QFont::Bold);

    for ( const auto &pattern : keywordPatterns ) {
        rule.pattern.setPattern(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    //Functions
    rule.pattern.setPattern("\\b[A-Za-z0-9_]+(?=\\()");
    rule.format.clearBackground();
    rule.format.clearForeground();
    rule.format.setFontItalic(true);
    rule.format.setForeground(Qt::darkCyan);
    highlightingRules.append(rule);

    //single-line comment
    rule.pattern.setPattern("//[^\n]*");
    rule.format.clearBackground();
    rule.format.clearForeground();
    rule.format.setForeground(Qt::gray);
    highlightingRules.append(rule);

    //quotation
    rule.pattern.setPattern("\".*\"");
    rule.format.clearBackground();
    rule.format.clearForeground();
    rule.format.setForeground(Qt::darkGreen);
    highlightingRules.append(rule);

    multiLineCommentFormat.setForeground(Qt::gray);
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    for ( const auto &it : highlightingRules ) {
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
            commentLength = endIndex - startIndex
                            + match.capturedLength();
        }

        setFormat(startIndex, commentLength, multiLineCommentFormat);
        startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
    }
}
