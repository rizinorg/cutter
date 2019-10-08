#ifndef ASCIIHIGHLIGHTER_H
#define ASCIIHIGHLIGHTER_H

#include <QSyntaxHighlighter>

#include <QHash>
#include <QTextCharFormat>

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
        QString pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QString commentStartRegularExpression;
    QString commentEndRegularExpression;

    QTextCharFormat keywordFormat;
    QTextCharFormat classFormat;
    QTextCharFormat singleLineCommentFormat;
    QTextCharFormat multiLineCommentFormat;
    QTextCharFormat quotationFormat;
    QTextCharFormat asciiFormat;
};

#endif   // ASCIIHIGHLIGHTER_H
