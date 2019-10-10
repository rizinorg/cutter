#ifndef ASCIIHIGHLIGHTER_H
#define ASCIIHIGHLIGHTER_H

#include <QSyntaxHighlighter>

#include <QHash>
#include <QTextCharFormat>
#include <QRegularExpression>

class QTextDocument;

class AsciiHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit AsciiHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text);

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QRegularExpression commentStartRegularExpression;
    QRegularExpression commentEndRegularExpression;

    QTextCharFormat keywordFormat;
    QTextCharFormat classFormat;
    QTextCharFormat singleLineCommentFormat;
    QTextCharFormat multiLineCommentFormat;
    QTextCharFormat quotationFormat;
    QTextCharFormat asciiFormat;
};

#endif   // ASCIIHIGHLIGHTER_H
