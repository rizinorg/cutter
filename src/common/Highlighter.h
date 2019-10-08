#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include "core/Cutter.h"

#include <QSyntaxHighlighter>
#include <QHash>
#include <QTextCharFormat>
#include <QRegularExpression>

class QTextDocument;
class MainWindow;

class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    Highlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text);

private:
    CutterCore *core;

    struct HighlightingRule {
        QString pattern;
        QRegularExpression::PatternOptions options;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QString commentStartRegularExpression;
    QString commentEndRegularExpression;

    QTextCharFormat keywordFormat;
    QTextCharFormat regFormat;
    QTextCharFormat classFormat;
    QTextCharFormat singleLineCommentFormat;
    QTextCharFormat multiLineCommentFormat;
    QTextCharFormat quotationFormat;
    QTextCharFormat functionFormat;
};

#endif   // HIGHLIGHTER_H
