#include "CodeColorParse.h"
#include "SynataxColor.h"

#ifdef LoadImage
#    undef LoadImage
#endif

#include <funcdata.hh>
#include <r_util.h>
#include <pugixml.hpp>
#include <sstream>
#include <string>

struct ParseCodeXMLContext
{
    Funcdata *func;
    std::map<uintm, PcodeOp *> ops;

    explicit ParseCodeXMLContext(Funcdata *func) : func(func)
    {
        for (auto it = func->beginOpAll(); it != func->endOpAll(); it++)
            ops[it->first.getTime()] = it->second;
    }
};

#define ANNOTATOR_PARAMS                                                                           \
    pugi::xml_node node, ParseCodeXMLContext *ctx, std::vector<RCodeAnnotation> *out
#define ANNOTATOR [](ANNOTATOR_PARAMS) -> void

void AnnotateOpref(ANNOTATOR_PARAMS)
{
    pugi::xml_attribute attr = node.attribute("opref");
    if (attr.empty())
        return;
    unsigned long long opref = attr.as_ullong(ULLONG_MAX);
    if (opref == ULLONG_MAX)
        return;
    auto opit = ctx->ops.find((uintm)opref);
    if (opit == ctx->ops.end())
        return;
    auto op = opit->second;

    out->emplace_back();
    auto &annotation = out->back();
    annotation = {};
    annotation.type = R_CODE_ANNOTATION_TYPE_OFFSET;
    annotation.offset.offset = op->getAddr().getOffset();
}

/**
 * Translate Ghidra's color annotations, which are essentially
 * loose token classes of the high level decompiled source code.
 **/
void AnnotateColor(ANNOTATOR_PARAMS)
{
    pugi::xml_attribute attr = node.attribute("color");
    if (attr.empty())
        return;

    std::string color = attr.as_string();
    if (color == "")
        return;

    RSyntaxHighlightType type;
    if (color == "keyword")
        type = R_SYNTAX_HIGHLIGHT_TYPE_KEYWORD;
    else if (color == "comment")
        type = R_SYNTAX_HIGHLIGHT_TYPE_COMMENT;
    else if (color == "type")
        type = R_SYNTAX_HIGHLIGHT_TYPE_DATATYPE;
    else if (color == "funcname")
        type = R_SYNTAX_HIGHLIGHT_TYPE_FUNCTION_NAME;
    else if (color == "param")
        type = R_SYNTAX_HIGHLIGHT_TYPE_FUNCTION_PARAMETER;
    else if (color == "var")
        type = R_SYNTAX_HIGHLIGHT_TYPE_LOCAL_VARIABLE;
    else if (color == "const")
        type = R_SYNTAX_HIGHLIGHT_TYPE_CONSTANT_VARIABLE;
    else if (color == "global")
        type = R_SYNTAX_HIGHLIGHT_TYPE_GLOBAL_VARIABLE;
    else
        return;
    RCodeAnnotation annotation = {};
    annotation.type = R_CODE_ANNOTATION_TYPE_SYNTAX_HIGHLIGHT;
    annotation.syntax_highlight.type = type;
    out->push_back(annotation);
}

static const std::map<std::string, std::vector<void (*)(ANNOTATOR_PARAMS)>> annotators = {
    { "statement", { AnnotateOpref } }, { "op", { AnnotateOpref, AnnotateColor } },
    { "comment", { AnnotateColor } },   { "variable", { AnnotateColor } },
    { "funcname", { AnnotateColor } },  { "type", { AnnotateColor } },
    { "syntax", { AnnotateColor } }
};

//#define TEST_UNKNOWN_NODES

/**
 * Ghidra returns an annotated AST of the decompiled high-level language code.
 * The AST is saved in XML format.
 *
 * This function is a DFS traversal over Ghidra's AST.
 * It parses some of the annotatations (e.g. decompilation offsets, token classes, ..)
 * and translates them into a suitable format
 * that can be natively saved in the RAnnotatedCode structure.
 **/
static void ParseNode(pugi::xml_node node, ParseCodeXMLContext *ctx, std::ostream &stream,
                      RAnnotatedCode *code)
{
    // A leaf is an XML node which contains parts of the high level decompilation language
    if (node.type() == pugi::xml_node_type::node_pcdata) {
        stream << node.value();
        return;
    }

    std::vector<RCodeAnnotation> annotations;
#ifdef TEST_UNKNOWN_NODES
    bool close_test = false;
    static const std::set<std::string> boring_tags = { "syntax" };
#endif

    if (strcmp(node.name(), "break") == 0) {
        stream << "\n";
        stream << std::string(node.attribute("indent").as_uint(0), ' ');
    } else {
        auto it = annotators.find(node.name());
        if (it != annotators.end()) {
            auto &callbacks = it->second;
            for (auto &callback : callbacks)
                callback(node, ctx, &annotations);
            for (auto &annotation : annotations)
                annotation.start = stream.tellp();
        }
#ifdef TEST_UNKNOWN_NODES
        else if (boring_tags.find(node.name()) == boring_tags.end()) {
            close_test = true;
            stream << "<" << node.name();
            for (pugi::xml_attribute attr : node.attributes())
                stream << " " << attr.name() << "=\"" << attr.value()
                       << "\""; // unescaped, but who cares
            stream << ">";
        }
#endif
    }

    for (pugi::xml_node child : node)
        ParseNode(child, ctx, stream, code);

    // an annotation applies for a node an all its children
    for (auto &annotation : annotations) {
        annotation.end = stream.tellp();
        r_annotated_code_add_annotation(code, &annotation);
    }

#ifdef TEST_UNKNOWN_NODES
    if (close_test)
        stream << "</" << node.name() << ">";
#endif
}

R_API RAnnotatedCode *ParseCodeXML(Funcdata *func, const char *xml)
{
    pugi::xml_document doc;
    if (!doc.load_string(xml, pugi::parse_default | pugi::parse_ws_pcdata))
        return nullptr;

    std::stringstream ss;
    RAnnotatedCode *code = r_annotated_code_new(nullptr);
    if (!code)
        return nullptr;

    ParseCodeXMLContext ctx(func);
    ParseNode(doc.child("function"), &ctx, ss, code);

    std::string str = ss.str();
    code->code = reinterpret_cast<char *>(r_malloc(str.length() + 1));
    if (!code->code) {
        r_annotated_code_free(code);
        return nullptr;
    }
    memcpy(code->code, str.c_str(), str.length());
    code->code[str.length()] = '\0';
    return code;
}