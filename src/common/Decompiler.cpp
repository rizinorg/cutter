
#include "Decompiler.h"
#include "Cutter.h"

#include <QJsonObject>
#include <QJsonArray>

Decompiler::Decompiler(const QString &id, const QString &name, QObject *parent)
    : QObject(parent), id(id), name(name)
{
}

static char *jsonToStrdup(const CutterJson &str)
{
    const RzJson *j = str.lowLevelValue();
    if (!j || j->type != RZ_JSON_STRING) {
        return NULL;
    }
    return rz_str_new(j->str_value);
}

static RzAnnotatedCode *parseJsonCode(CutterJson &json)
{
    char *raw_code = jsonToStrdup(json["code"]);
    if (!raw_code) {
        return NULL;
    }
    RzAnnotatedCode *code = rz_annotated_code_new(raw_code);
    if (!code) {
        return NULL;
    }
    for (const auto &jsonAnnotation : json["annotations"]) {
        RzCodeAnnotation annotation = {};
        annotation.start = jsonAnnotation["start"].toUt64();
        annotation.end = jsonAnnotation["end"].toUt64();
        QString type = jsonAnnotation["type"].toString();
        if (type == "offset") {
            annotation.type = RZ_CODE_ANNOTATION_TYPE_OFFSET;
            annotation.offset.offset = jsonAnnotation["offset"].toString().toULongLong();
        } else if (type == "function_name") {
            annotation.type = RZ_CODE_ANNOTATION_TYPE_FUNCTION_NAME;
            annotation.reference.name = jsonToStrdup(jsonAnnotation["name"]);
            annotation.reference.offset = jsonAnnotation["offset"].toString().toULongLong();
        } else if (type == "global_variable") {
            annotation.type = RZ_CODE_ANNOTATION_TYPE_GLOBAL_VARIABLE;
            annotation.reference.offset = jsonAnnotation["offset"].toString().toULongLong();
        } else if (type == "constant_variable") {
            annotation.type = RZ_CODE_ANNOTATION_TYPE_CONSTANT_VARIABLE;
            annotation.reference.offset = jsonAnnotation["offset"].toString().toULongLong();
        } else if (type == "local_variable") {
            annotation.type = RZ_CODE_ANNOTATION_TYPE_LOCAL_VARIABLE;
            annotation.variable.name = jsonToStrdup(jsonAnnotation["name"]);
        } else if (type == "function_parameter") {
            annotation.type = RZ_CODE_ANNOTATION_TYPE_FUNCTION_PARAMETER;
            annotation.variable.name = jsonToStrdup(jsonAnnotation["name"]);
        } else if (type == "syntax_highlight") {
            annotation.type = RZ_CODE_ANNOTATION_TYPE_SYNTAX_HIGHLIGHT;
            QString highlightType = jsonAnnotation["syntax_highlight"].toString();
            if (highlightType == "keyword") {
                annotation.syntax_highlight.type = RZ_SYNTAX_HIGHLIGHT_TYPE_KEYWORD;
            } else if (highlightType == "comment") {
                annotation.syntax_highlight.type = RZ_SYNTAX_HIGHLIGHT_TYPE_COMMENT;
            } else if (highlightType == "datatype") {
                annotation.syntax_highlight.type = RZ_SYNTAX_HIGHLIGHT_TYPE_DATATYPE;
            } else if (highlightType == "function_name") {
                annotation.syntax_highlight.type = RZ_SYNTAX_HIGHLIGHT_TYPE_FUNCTION_NAME;
            } else if (highlightType == "function_parameter") {
                annotation.syntax_highlight.type = RZ_SYNTAX_HIGHLIGHT_TYPE_FUNCTION_PARAMETER;
            } else if (highlightType == "local_variable") {
                annotation.syntax_highlight.type = RZ_SYNTAX_HIGHLIGHT_TYPE_LOCAL_VARIABLE;
            } else if (highlightType == "constant_variable") {
                annotation.syntax_highlight.type = RZ_SYNTAX_HIGHLIGHT_TYPE_CONSTANT_VARIABLE;
            } else if (highlightType == "global_variable") {
                annotation.syntax_highlight.type = RZ_SYNTAX_HIGHLIGHT_TYPE_GLOBAL_VARIABLE;
            }
        }
        rz_annotated_code_add_annotation(code, &annotation);
    }
    return code;
}

RzAnnotatedCode *Decompiler::makeWarning(QString warningMessage)
{
    std::string temporary = warningMessage.toStdString();
    return rz_annotated_code_new(strdup(temporary.c_str()));
}

JSDecDecompiler::JSDecDecompiler(QObject *parent) : Decompiler("jsdec", "jsdec", parent)
{
    task = nullptr;
}

bool JSDecDecompiler::isAvailable()
{
    return Core()->cmdList("es").contains("jsdec");
}

void JSDecDecompiler::decompileAt(RVA addr)
{
    if (task) {
        return;
    }
    task = new RizinCmdTask("pddA @ " + QString::number(addr));
    connect(task, &RizinCmdTask::finished, this, [this]() {
        CutterJson json = task->getResultJson();
        delete task;
        task = nullptr;
        if (!json.size()) {
            emit finished(Decompiler::makeWarning(tr("Failed to parse JSON from jsdec")));
            return;
        }
        RzAnnotatedCode *code = parseJsonCode(json);
        emit finished(code);
    });
    task->startTask();
}
