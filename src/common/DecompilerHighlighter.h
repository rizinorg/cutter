#ifndef DECOMPILER_HIGHLIGHTER_H
#define DECOMPILER_HIGHLIGHTER_H

#include "CutterCommon.h"
#include <rz_util/rz_annotated_code.h>
#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QTextCharFormat>
#include <array>

/**
 * \brief SyntaxHighlighter based on annotations from decompiled code.
 * Can be only used in combination with DecompilerWidget.
 */
class CUTTER_EXPORT DecompilerHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    DecompilerHighlighter(QTextDocument *parent = nullptr);
    virtual ~DecompilerHighlighter() = default;

    /**
     * @brief Set the code with annotations to be used for highlighting.
     *
     * It is callers responsibility to ensure that it is synchronized with currentTextDocument and
     * has sufficiently long lifetime.
     *
     * @param code
     */
    void setAnnotations(RzAnnotatedCode *code);

protected:
    void highlightBlock(const QString &text) override;

private:
    void setupTheme();

    static const int HIGHLIGHT_COUNT = RZ_SYNTAX_HIGHLIGHT_TYPE_GLOBAL_VARIABLE + 1;
    std::array<QTextCharFormat, HIGHLIGHT_COUNT> format;
    RzAnnotatedCode *code = nullptr;
};

#endif
