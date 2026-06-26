#ifndef MDHIGHLIGHTER_H
#define MDHIGHLIGHTER_H

#include <QHash>
#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

class QTextDocument;

/**
 * @brief Wrapper of QSyntaxHighlighter for Markdown
 */
class MdHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit MdHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text);

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QTextCharFormat keywordFormat;
    QTextCharFormat classFormat;
    QTextCharFormat singleLineCommentFormat;
    QTextCharFormat multiLineCommentFormat;
    QTextCharFormat quotationFormat;
    QTextCharFormat functionFormat;
};

#endif // MDHIGHLIGHTER_H
