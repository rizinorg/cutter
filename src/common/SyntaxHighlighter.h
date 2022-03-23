#ifndef SYNTAXHIGHLIGHTER_H
#define SYNTAXHIGHLIGHTER_H

#include "CutterCommon.h"
#include <QSyntaxHighlighter>
#include <QVector>
#include <QTextDocument>
#include <QRegularExpression>
#include <QTextCharFormat>

#ifdef CUTTER_ENABLE_KSYNTAXHIGHLIGHTING

#    include <KSyntaxHighlighting/SyntaxHighlighter>

class SyntaxHighlighter : public KSyntaxHighlighting::SyntaxHighlighter
{
    Q_OBJECT

public:
    SyntaxHighlighter(QTextDocument *document);

private slots:
    void updateTheme();
};

#endif

/**
 * SyntaxHighlighter to be used when KSyntaxHighlighting is not available
 */
class CUTTER_EXPORT FallbackSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    FallbackSyntaxHighlighter(QTextDocument *parent = nullptr);
    virtual ~FallbackSyntaxHighlighter() = default;

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    QVector<HighlightingRule> highlightingRules;

    QRegularExpression commentStartExpression;
    QRegularExpression commentEndExpression;

    QTextCharFormat multiLineCommentFormat;
};

#endif
