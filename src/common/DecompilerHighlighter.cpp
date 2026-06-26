#include "DecompilerHighlighter.h"

#include "common/Configuration.h"

#include <memory>

DecompilerHighlighter::DecompilerHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent)
{
    setupTheme();
    connect(Config(), &Configuration::colorsUpdated, this, [this]() {
        setupTheme();
        rehighlight();
    });
}

void DecompilerHighlighter::setAnnotations(RzAnnotatedCode *code)
{
    this->code = code;
}

void DecompilerHighlighter::setupTheme()
{
    const struct
    {
        RSyntaxHighlightType type;
        QString name;
    } mapping[] = {
        { RZ_SYNTAX_HIGHLIGHT_TYPE_KEYWORD, "pop" },
        { RZ_SYNTAX_HIGHLIGHT_TYPE_COMMENT, "comment" },
        { RZ_SYNTAX_HIGHLIGHT_TYPE_DATATYPE, "func_var_type" },
        { RZ_SYNTAX_HIGHLIGHT_TYPE_FUNCTION_NAME, "fname" },
        { RZ_SYNTAX_HIGHLIGHT_TYPE_FUNCTION_PARAMETER, "args" },
        { RZ_SYNTAX_HIGHLIGHT_TYPE_LOCAL_VARIABLE, "func_var" },
        { RZ_SYNTAX_HIGHLIGHT_TYPE_CONSTANT_VARIABLE, "num" },
        { RZ_SYNTAX_HIGHLIGHT_TYPE_GLOBAL_VARIABLE, "flag" },
    };
    for (const auto &pair : mapping) {
        assert(pair.type < format.size());
        format[pair.type].setForeground(Config()->getColor(pair.name));
    }
}

void DecompilerHighlighter::highlightBlock(const QString &)
{
    if (!code) {
        return;
    }
    auto block = currentBlock();
    const size_t start = block.position();
    const size_t end = block.position() + block.length();

    auto annotations = fromOwned(rz_annotated_code_annotations_range(code, start, end));
    for (const auto *annotation : CutterPVector<RzCodeAnnotation>(annotations.get())) {

        if (annotation->type != RZ_CODE_ANNOTATION_TYPE_SYNTAX_HIGHLIGHT) {
            continue;
        }
        auto type = annotation->syntax_highlight.type;
        if (size_t(type) >= highlightCount) {
            continue;
        }
        auto annotationStart = annotation->start;
        if (annotationStart < start) {
            annotationStart = 0;
        } else {
            annotationStart -= start;
        }
        auto annotationEnd = annotation->end - start;

        setFormat(annotationStart, annotationEnd - annotationStart, format[type]);
    }
}
