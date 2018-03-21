#pragma once

#include <QSyntaxHighlighter>
#include <QVector>
#include <QTextDocument>
#include <QRegularExpression>
#include <QTextCharFormat>


class SyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    SyntaxHighlighter(QTextDocument *parent = nullptr);
    virtual ~SyntaxHighlighter() = default;

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    QVector<HighlightingRule> highlightingRules;

    QRegularExpression commentStartExpression;
    QRegularExpression commentEndExpression;

    QTextCharFormat multiLineCommentFormat;
};
