#include "SyntaxColor.h"

#include <r_util.h>
#include <r_cons.h>

R_API RAnnotatedCode *r_annotated_code_new(char *code)
{
    RAnnotatedCode *r = R_NEW0(RAnnotatedCode);
    if (!r) {
        return NULL;
    }
    r->code = code;
    r_vector_init(&r->annotations, sizeof(RCodeAnnotation), NULL, NULL);
    return r;
}

R_API void r_annotated_code_free(RAnnotatedCode *code)
{
    r_vector_clear(&code->annotations);
    r_free(code->code);
    r_free(code);
}

R_API void r_annotated_code_add_annotation(RAnnotatedCode *code, RCodeAnnotation *annotation)
{
    r_vector_push(&code->annotations, annotation);
}

R_API RPVector *r_annotated_code_annotations_in(RAnnotatedCode *code, size_t offset)
{
    RPVector *r = r_pvector_new(NULL);
    if (!r) {
        return NULL;
    }
    RCodeAnnotation *annotation;
    r_vector_foreach(&code->annotations, annotation)
    {
        if (offset >= annotation->start && offset < annotation->end) {
            r_pvector_push(r, annotation);
        }
    }
    return r;
}

R_API void r_annotated_code_print_json(RAnnotatedCode *code)
{
    PJ *pj = pj_new();
    if (!pj) {
        return;
    }

    pj_o(pj);
    pj_ks(pj, "code", code->code);

    pj_k(pj, "annotations");
    pj_a(pj);

    char *type_str;
    RCodeAnnotation *annotation;
    r_vector_foreach(&code->annotations, annotation)
    {
        pj_o(pj);
        pj_kn(pj, "start", (ut64)annotation->start);
        pj_kn(pj, "end", (ut64)annotation->end);
        switch (annotation->type) {
        case R_CODE_ANNOTATION_TYPE_OFFSET:
            pj_ks(pj, "type", "offset");
            pj_kn(pj, "offset", annotation->offset.offset);
            break;
        case R_CODE_ANNOTATION_TYPE_SYNTAX_HIGHLIGHT:
            pj_ks(pj, "type", "syntax_highlight");
            switch (annotation->syntax_highlight.type) {
            case R_SYNTAX_HIGHLIGHT_TYPE_KEYWORD:
                type_str = "keyword";
                break;
            case R_SYNTAX_HIGHLIGHT_TYPE_COMMENT:
                type_str = "comment";
                break;
            case R_SYNTAX_HIGHLIGHT_TYPE_DATATYPE:
                type_str = "datatype";
                break;
            case R_SYNTAX_HIGHLIGHT_TYPE_FUNCTION_NAME:
                type_str = "function_name";
                break;
            case R_SYNTAX_HIGHLIGHT_TYPE_FUNCTION_PARAMETER:
                type_str = "function_parameter";
                break;
            case R_SYNTAX_HIGHLIGHT_TYPE_LOCAL_VARIABLE:
                type_str = "local_variable";
                break;
            case R_SYNTAX_HIGHLIGHT_TYPE_CONSTANT_VARIABLE:
                type_str = "constant_variable";
                break;
            case R_SYNTAX_HIGHLIGHT_TYPE_GLOBAL_VARIABLE:
                type_str = "global_variable";
                break;
            }
            pj_ks(pj, "syntax_highlight", type_str);
            break;
        }
        pj_end(pj);
    }
    pj_end(pj);

    pj_end(pj);
    r_cons_printf("%s\n", pj_string(pj));
    pj_free(pj);
}

#define PALETTE(x) (cons && cons->context->pal.x) ? cons->context->pal.x
#define PICK_COLOR(x) ((code->color_enabled) ? (x) : "")

static void print_offset_in_binary_line_bar(RAnnotatedCode *code, ut64 offset)
{
    RCons *cons = r_cons_singleton();
    if (!offset) {
        r_cons_printf("                  |");
    } else {
        char hex_str[0x10];
        snprintf(hex_str, 11, "0x%08x" PRIx64, offset);
        r_cons_printf("    ");
        r_cons_printf("%s", PICK_COLOR(PALETTE(offset) : Color_GREEN));
        r_cons_printf("%s", hex_str);
        r_cons_printf("%s", PICK_COLOR(Color_RESET));
        r_cons_printf("    |");
    }
}

R_API void r_annotated_code_print(RAnnotatedCode *code, RVector *line_offsets)
{
    if (code->annotations.len == 0) {
        r_cons_printf("%s\n", code->code);
        return;
    }

    size_t cur = 0;
    size_t line_idx = 0;
    size_t len = strlen(code->code);

    RCons *cons = r_cons_singleton();
    RCodeAnnotation *annotation;
    r_vector_foreach(&code->annotations, annotation)
    {
        if (annotation->type != R_CODE_ANNOTATION_TYPE_SYNTAX_HIGHLIGHT) {
            continue;
        }

        // (1/3)
        // now we have a syntax highlighting annotation.
        // pick a suitable color for it.
        const char *color = Color_RESET;
        switch (annotation->syntax_highlight.type) {
        case R_SYNTAX_HIGHLIGHT_TYPE_COMMENT:
            color = PALETTE(comment) : Color_WHITE;
            break;
        case R_SYNTAX_HIGHLIGHT_TYPE_KEYWORD:
            color = PALETTE(pop) : Color_MAGENTA;
            break;
        case R_SYNTAX_HIGHLIGHT_TYPE_DATATYPE:
            color = PALETTE(func_var_type) : Color_BLUE;
            break;
        case R_SYNTAX_HIGHLIGHT_TYPE_FUNCTION_NAME:
            color = PALETTE(fname) : Color_RED;
            break;
        case R_SYNTAX_HIGHLIGHT_TYPE_CONSTANT_VARIABLE:
            color = PALETTE(num) : Color_YELLOW;
        default:
            break;
        }

        // (2/3)
        // the chunk before the syntax highlighting annotation should not be colored
        for (; cur < annotation->start && cur < len; cur++) {
            // if we are starting a new line and we are printing with offsets
            // we need to prepare the bar with offsets on the left handside before that
            if (line_offsets && (cur == 0 || code->code[cur - 1] == '\n')) {
                ut64 offset = 0;
                if (line_idx < line_offsets->len) {
                    offset = *(ut64 *)r_vector_index_ptr(line_offsets, line_idx);
                }
                print_offset_in_binary_line_bar(code, offset);
                line_idx++;
            }
            r_cons_printf("%c", code->code[cur]);
        }

        // (3/3)
        // everything in between the "start" and the "end" inclusive should be highlighted
        r_cons_printf("%s", PICK_COLOR(color));
        for (; cur < annotation->end && cur < len; cur++) {
            // if we are starting a new line and we are printing with offsets
            // we need to prepare the bar with offsets on the left handside before that
            if (line_offsets && (cur == 0 || code->code[cur - 1] == '\n')) {
                ut64 offset = 0;
                if (line_idx < line_offsets->len) {
                    offset = *(ut64 *)r_vector_index_ptr(line_offsets, line_idx);
                }
                r_cons_printf("%s", PICK_COLOR(Color_RESET));
                print_offset_in_binary_line_bar(code, offset);
                r_cons_printf("%s", PICK_COLOR(color));
                line_idx++;
            }
            r_cons_printf("%c", code->code[cur]);
        }
        r_cons_printf("%s", PICK_COLOR(Color_RESET));
    }
    // the rest of the decompiled code should be printed
    // without any highlighting since we don't have any annotations left
    for (; cur < len; cur++) {
        // if we are starting a new line and we are printing with offsets
        // we need to prepare the bar with offsets on the left handside before that
        if (line_offsets && (cur == 0 || code->code[cur - 1] == '\n')) {
            ut64 offset = 0;
            if (line_idx < line_offsets->len) {
                offset = *(ut64 *)r_vector_index_ptr(line_offsets, line_idx);
            }
            print_offset_in_binary_line_bar(code, offset);
            line_idx++;
        }
        r_cons_printf("%c", code->code[cur]);
    }
}