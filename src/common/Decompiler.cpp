
#include "Decompiler.h"
#include "Cutter.h"

#include <QJsonObject>
#include <QJsonArray>

Decompiler::Decompiler(const QString &id, const QString &name, QObject *parent)
    : QObject(parent), id(id), name(name)
{
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
    task = new RizinCmdTask("pddj @ " + QString::number(addr));
    connect(task, &RizinCmdTask::finished, this, [this]() {
        CutterJson json = task->getResultJson();
        delete task;
        task = nullptr;
        if (!json.size()) {
            emit finished(Decompiler::makeWarning(tr("Failed to parse JSON from jsdec")));
            return;
        }
        RzAnnotatedCode *code = rz_annotated_code_new(nullptr);
        QString codeString = "";
        for (auto line : json["log"]) {
            if (line.type() != RZ_JSON_STRING) {
                continue;
            }
            codeString.append(line.toString() + "\n");
        }

        for (auto lineObject : json["lines"]) {
            if (!lineObject.size()) {
                continue;
            }
            RzCodeAnnotation annotationi = {};
            annotationi.start = codeString.length();
            codeString.append(lineObject["str"].toString() + "\n");
            annotationi.end = codeString.length();
            annotationi.type = RZ_CODE_ANNOTATION_TYPE_OFFSET;
            annotationi.offset.offset = lineObject["offset"].toUt64();
            rz_annotated_code_add_annotation(code, &annotationi);
        }

        for (auto line : json["errors"]) {
            if (line.type() != RZ_JSON_STRING) {
                continue;
            }
            codeString.append(line.toString() + "\n");
        }
        std::string tmp = codeString.toStdString();
        code->code = strdup(tmp.c_str());
        emit finished(code);
    });
    task->startTask();
}
