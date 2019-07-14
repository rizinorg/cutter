
#include "Decompiler.h"
#include "Cutter.h"

#include <QJsonObject>
#include <QJsonArray>

Decompiler::Decompiler(const QString &id, const QString &name, QObject *parent)
    : QObject(parent),
    id(id),
    name(name)
{
}

R2DecDecompiler::R2DecDecompiler(QObject *parent)
    : Decompiler("r2dec", "r2dec", parent)
{
}

bool R2DecDecompiler::isAvailable()
{
    return Core()->cmdList("e cmd.pdc=?").contains(QStringLiteral("pdd"));
}

DecompiledCode R2DecDecompiler::decompileAt(RVA addr)
{
    DecompiledCode code;
    QString s;

    QJsonObject json = Core()->cmdj("pddj @ " + QString::number(addr)).object();
    if (json.isEmpty()) {
        return code;
    }

    for (const auto &line : json["log"].toArray()) {
        if (!line.isString()) {
            continue;
        }
        code.lines.append(DecompiledCode::Line(line.toString()));
    }

    auto linesArray = json["lines"].toArray();
    code.lines.reserve(code.lines.size() + linesArray.size());
    for (const auto &line : linesArray) {
        QJsonObject lineObject = line.toObject();
        if (lineObject.isEmpty()) {
            continue;
        }
        DecompiledCode::Line codeLine;
        codeLine.str = lineObject["str"].toString();
        bool ok;
        codeLine.addr = lineObject["offset"].toVariant().toULongLong(&ok);
        if (!ok) {
            codeLine.addr = RVA_INVALID;
        }
        code.lines.append(codeLine);
    }

    for (const auto &line : json["errors"].toArray()) {
        if (!line.isString()) {
            continue;
        }
        code.lines.append(DecompiledCode::Line(line.toString()));
    }

    return code;
}