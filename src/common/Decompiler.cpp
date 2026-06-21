#include "Decompiler.h"

#include "Cutter.h"

#include <QJsonArray>
#include <QJsonObject>

Decompiler::Decompiler(const QString &id, const QString &name, QObject *parent)
    : QObject(parent), id(id), name(name)
{
}

RzAnnotatedCode *Decompiler::makeWarning(const QString &warningMessage)
{
    const std::string temporary = warningMessage.toStdString();
    return rz_annotated_code_new(strdup(temporary.c_str()));
}
