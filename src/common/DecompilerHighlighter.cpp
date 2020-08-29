
#include "DecompilerHighlighter.h"
#include "common/Configuration.h"

DecompilerHighlighter::DecompilerHighlighter(QTextDocument *parent)
    :   QSyntaxHighlighter(parent)
{
    setupTheme();
    connect(Config(), &Configuration::colorsUpdated, this, [this]() {
        setupTheme();
        rehighlight();
    });
}

void DecompilerHighlighter::setAnnotations(RAnnotatedCode *code)
{
    this->code = code;
}

void DecompilerHighlighter::setupTheme()
{
    format[R_SYNTAX_HIGHLIGHT_TYPE_KEYWORD].setForeground(Config()->getColor("other"));
    format[R_SYNTAX_HIGHLIGHT_TYPE_COMMENT].setForeground(Config()->getColor("comment"));
    format[R_SYNTAX_HIGHLIGHT_TYPE_DATATYPE].setForeground(Config()->getColor("func_var_type"));
    format[R_SYNTAX_HIGHLIGHT_TYPE_FUNCTION_NAME].setForeground(Config()->getColor("fname"));
    format[R_SYNTAX_HIGHLIGHT_TYPE_FUNCTION_PARAMETER].setForeground(Config()->getColor("args"));
    format[R_SYNTAX_HIGHLIGHT_TYPE_LOCAL_VARIABLE].setForeground(Config()->getColor("func_var"));
    format[R_SYNTAX_HIGHLIGHT_TYPE_CONSTANT_VARIABLE].setForeground(Config()->getColor("num"));
    format[R_SYNTAX_HIGHLIGHT_TYPE_GLOBAL_VARIABLE].setForeground(Config()->getColor("func_var"));
}

void DecompilerHighlighter::highlightBlock(const QString &)
{
    if (!code) {
        return;
    }
    auto block = currentBlock();
    size_t start = block.position();
    size_t end = block.position() + block.length();

    std::unique_ptr<RPVector, typeof(&r_pvector_free)> annotations(r_annotated_code_annotations_range(code, start, end), &r_pvector_free);
    void **iter;
    r_pvector_foreach(annotations.get(), iter) {
        RCodeAnnotation *annotation = static_cast<RCodeAnnotation*>(*iter);
        if (annotation->type != R_CODE_ANNOTATION_TYPE_SYNTAX_HIGHLIGHT) {
            continue;
        }
        auto type = annotation->syntax_highlight.type;
        if (type >= HIGHLIGHT_COUNT) {
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
