
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
