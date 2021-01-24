
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
        QJsonObject json = task->getResultJson().object();
        delete task;
        task = nullptr;
        if (json.isEmpty()) {
            emit finished(Decompiler::makeWarning(tr("Failed to parse JSON from jsdec")));
            return;
        }
        RzAnnotatedCode *code = rz_annotated_code_new(nullptr);
        QString codeString = "";
        for (const auto &line : json["log"].toArray()) {
            if (!line.isString()) {
                continue;
            }
            codeString.append(line.toString() + "\n");
        }

        auto linesArray = json["lines"].toArray();
        for (const auto &line : linesArray) {
            QJsonObject lineObject = line.toObject();
            if (lineObject.isEmpty()) {
                continue;
            }
            RzCodeAnnotation annotationi = {};
            annotationi.start = codeString.length();
            codeString.append(lineObject["str"].toString() + "\n");
            annotationi.end = codeString.length();
            bool ok;
            annotationi.type = RZ_CODE_ANNOTATION_TYPE_OFFSET;
            annotationi.offset.offset = lineObject["offset"].toVariant().toULongLong(&ok);
            rz_annotated_code_add_annotation(code, &annotationi);
        }

        for (const auto &line : json["errors"].toArray()) {
            if (!line.isString()) {
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
