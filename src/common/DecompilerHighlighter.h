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

#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
#    define Q_DISABLE_COPY(DecompilerHighlighter)                                                  \
        DecompilerHighlighter(const DecompilerHighlighter &w) = delete;                            \
        DecompilerHighlighter &operator=(const DecompilerHighlighter &w) = delete;

#    define Q_DISABLE_MOVE(DecompilerHighlighter)                                                  \
        DecompilerHighlighter(DecompilerHighlighter &&w) = delete;                                 \
        DecompilerHighlighter &operator=(DecompilerHighlighter &&w) = delete;

#    define Q_DISABLE_COPY_MOVE(DecompilerHighlighter)                                             \
        Q_DISABLE_COPY(DecompilerHighlighter)                                                      \
        Q_DISABLE_MOVE(DecompilerHighlighter)
#endif

    Q_DISABLE_COPY_MOVE(DecompilerHighlighter)

public:
    DecompilerHighlighter(QTextDocument *parent = nullptr);
    ~DecompilerHighlighter() override;

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
