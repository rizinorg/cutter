
#include "Decompiler.h"
#include "Cutter.h"

#include <QJsonObject>
#include <QJsonArray>

Decompiler::Decompiler(const QString &id, const QString &name, QObject *parent)
    : QObject(parent), id(id), name(name)
{
}

static char *jsonValueToString(const QJsonValue &str)
{
    return strdup(str.toString().toStdString().c_str());
}

RzAnnotatedCode *Decompiler::rz_annotated_code_from_json(QJsonObject &json)
{
    RzAnnotatedCode *code = rz_annotated_code_new(nullptr);
    code->code = strdup(json["code"].toString().toStdString().c_str());
    for (const auto &iter : json["annotations"].toArray()) {
        QJsonObject jsonAnnotation = iter.toObject();
        RzCodeAnnotation annotation = {};
        annotation.start = jsonAnnotation["start"].toInt();
        annotation.end = jsonAnnotation["end"].toInt();
        QString type = jsonAnnotation["type"].toString();
        if (type == "offset") {
            annotation.type = RZ_CODE_ANNOTATION_TYPE_OFFSET;
            annotation.offset.offset = jsonAnnotation["offset"].toString().toULongLong();
        } else if (type == "function_name") {
            annotation.type = RZ_CODE_ANNOTATION_TYPE_FUNCTION_NAME;
            annotation.reference.name = jsonValueToString(jsonAnnotation["name"]);
            annotation.reference.offset = jsonAnnotation["offset"].toString().toULongLong();
        } else if (type == "global_variable") {
            annotation.type = RZ_CODE_ANNOTATION_TYPE_GLOBAL_VARIABLE;
            annotation.reference.offset = jsonAnnotation["offset"].toString().toULongLong();
        } else if (type == "constant_variable") {
            annotation.type = RZ_CODE_ANNOTATION_TYPE_CONSTANT_VARIABLE;
            annotation.reference.offset = jsonAnnotation["offset"].toString().toULongLong();
        } else if (type == "local_variable") {
            annotation.type = RZ_CODE_ANNOTATION_TYPE_LOCAL_VARIABLE;
            annotation.variable.name = jsonValueToString(jsonAnnotation["name"]);
        } else if (type == "function_parameter") {
            annotation.type = RZ_CODE_ANNOTATION_TYPE_FUNCTION_PARAMETER;
            annotation.variable.name = jsonValueToString(jsonAnnotation["name"]);
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
        QJsonObject json = task->getResultJson().object();
        delete task;
        task = nullptr;
        if (json.isEmpty()) {
            emit finished(Decompiler::makeWarning(tr("Failed to parse JSON from jsdec")));
            return;
        }
        RzAnnotatedCode *code = this->rz_annotated_code_from_json(json);
        emit finished(code);
    });
    task->startTask();
}
