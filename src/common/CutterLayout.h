#ifndef CUTTER_LAYOUT_H
#define CUTTER_LAYOUT_H

#include <QByteArray>
#include <QMap>
#include <QString>
#include <QVariantMap>

namespace Cutter
{

struct CutterLayout
{
    QByteArray geometry;
    QByteArray state;
    QMap<QString, QVariantMap> viewProperties;
};

const QString LAYOUT_DEFAULT = "Default";
const QString LAYOUT_DEBUG = "Debug";

bool isBuiltinLayoutName(const QString &name);

}
#endif
