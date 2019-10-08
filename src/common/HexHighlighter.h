#ifndef HEXHIGHLIGHTER_H
#define HEXHIGHLIGHTER_H

#include <QSyntaxHighlighter>

#include <QHash>
#include <QTextCharFormat>
#include <QRegularExpression>

class QTextDocument;

class HexHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit HexHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text);

private:
    struct HighlightingRule {
        QString pattern;
        QRegularExpression::PatternOptions options;
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
    QTextCharFormat functionFormat;
};

#endif // HEXHIGHLIGHTER_H
