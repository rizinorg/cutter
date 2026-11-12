
#include "SyntaxHighlighter.h"

#ifdef CUTTER_ENABLE_KSYNTAXHIGHLIGHTING

#    include "Configuration.h"

#    include <KSyntaxHighlighting/Theme>

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *document)
    : KSyntaxHighlighting::SyntaxHighlighter(document)
{
    connect(Config(), &Configuration::kSyntaxHighlightingThemeChanged, this,
            &SyntaxHighlighter::updateTheme);
    updateTheme();
}

void SyntaxHighlighter::updateTheme()
{
    setTheme(Config()->getKSyntaxHighlightingTheme());
    rehighlight();
}

#endif

QStringList FallbackSyntaxHighlighter::keywords() const
{
    return {
        "auto",   "break",  "case",     "char",   "const",    "continue", "default",  "do",
        "double", "else",   "enum",     "extern", "float",    "for",      "goto",     "if",
        "int",    "long",   "register", "return", "short",    "signed",   "sizeof",   "static",
        "struct", "switch", "typedef",  "union",  "unsigned", "void",     "volatile", "while"
    };
}

QColor FallbackSyntaxHighlighter::keywordColor() const
{
    return QColor(80, 200, 215);
}

QColor FallbackSyntaxHighlighter::functionColor() const
{
    return Qt::darkCyan;
}

QColor FallbackSyntaxHighlighter::commentColor() const
{
    return Qt::gray;
}

QColor FallbackSyntaxHighlighter::stringColor() const
{
    return Qt::darkGreen;
}

FallbackSyntaxHighlighter::FallbackSyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent), commentStartExpression("/\\*"), commentEndExpression("\\*/")
{
    HighlightingRule rule;

    QTextCharFormat keywordFormat;
    keywordFormat.setForeground(keywordColor());

    const QStringList words = keywords();
    for (const QString &keyword : words) {
        rule.pattern.setPattern(QString("\\b%1\\b").arg(keyword));
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    // Functions
    rule.pattern.setPattern("\\b[A-Za-z0-9_]+(?=\\()");
    rule.format.clearBackground();
    rule.format.clearForeground();
    rule.format.setFontItalic(true);
    rule.format.setForeground(functionColor());
    highlightingRules.append(rule);

    // single-line comment
    rule.pattern.setPattern("//[^\n]*");
    rule.format.clearBackground();
    rule.format.clearForeground();
    rule.format.setForeground(commentColor());
    highlightingRules.append(rule);

    // quotation
    rule.pattern.setPattern("\".*\"");
    rule.format.clearBackground();
    rule.format.clearForeground();
    rule.format.setForeground(stringColor());
    highlightingRules.append(rule);

    multiLineCommentFormat.setForeground(commentColor());
}

void FallbackSyntaxHighlighter::highlightBlock(const QString &text)
{
    for (const auto &it : std::as_const(highlightingRules)) {
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
