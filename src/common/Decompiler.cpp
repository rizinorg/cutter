
#include "Decompiler.h"
#include "Cutter.h"

#include <QJsonObject>
#include <QJsonArray>


ut64 AnnotatedCode::OffsetForPosition(size_t pos) const
{
    size_t closestPos = SIZE_MAX;
    ut64 closestOffset = UT64_MAX;
    for (const auto &annotation : annotations) {
        if (annotation.type != CodeAnnotation::Type::Offset || annotation.start > pos || annotation.end <= pos) {
            continue;
        }
        if (closestPos != SIZE_MAX && closestPos >= annotation.start) {
            continue;
        }
        closestPos = annotation.start;
        closestOffset = annotation.offset.offset;
    }
    return closestOffset;
}

size_t AnnotatedCode::PositionForOffset(ut64 offset) const
{
    size_t closestPos = SIZE_MAX;
    ut64 closestOffset = UT64_MAX;
    for (const auto &annotation : annotations) {
        if (annotation.type != CodeAnnotation::Type::Offset || annotation.offset.offset > offset) {
            continue;
        }
        if (closestOffset != UT64_MAX && closestOffset >= annotation.offset.offset) {
            continue;
        }
        closestPos = annotation.start;
        closestOffset = annotation.offset.offset;
    }
    return closestPos;
}


Decompiler::Decompiler(const QString &id, const QString &name, QObject *parent)
    : QObject(parent),
    id(id),
    name(name)
{
}

R2DecDecompiler::R2DecDecompiler(QObject *parent)
    : Decompiler("r2dec", "r2dec", parent)
{
    task = nullptr;
}

bool R2DecDecompiler::isAvailable()
{
    return Core()->cmdList("e cmd.pdc=?").contains(QStringLiteral("pdd"));
}

void R2DecDecompiler::decompileAt(RVA addr)
{
    if (task) {
        return;
    }
    task = new R2Task("pddj @ " + QString::number(addr));
    connect(task, &R2Task::finished, this, [this]() {
        RAnnotatedCode *codi = r_annotated_code_new (nullptr);
        QString codeString = "";
        AnnotatedCode code = {};
        QJsonObject json = task->getResultJson().object();
        delete task;
        task = nullptr;
        if (json.isEmpty()) {
            codeString = tr("Failed to parse JSON from r2dec");
            QByteArray ba = codeString.toUtf8();
            codi->code = ba.data();
            emit finished(codi);
            return;
        }

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
            RCodeAnnotation *annotationi = new RCodeAnnotation;
            annotationi->start = codeString.length();
            codeString.append(lineObject["str"].toString() + "\n");
            annotationi->end = codeString.length();
            bool ok;
            annotationi->type = R_CODE_ANNOTATION_TYPE_OFFSET;
            annotationi->offset.offset = lineObject["offset"].toVariant().toULongLong(&ok);
            r_annotated_code_add_annotation (codi, annotationi);
        }

        for (const auto &line : json["errors"].toArray()) {
            if (!line.isString()) {
                continue;
            }
            codeString.append(line.toString() + "\n");
        }
        QByteArray ba = codeString.toUtf8();
        codi->code = ba.data();
        emit finished(codi);
    });
    task->startTask();
}
