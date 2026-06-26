#ifndef JSON_H
#define JSON_H

#include "CutterCommon.h"

#include <QJsonValue>
#include <QVariant>

class QTreeWidgetItem;
class CutterJson;

/**
 * @file Json.h
 * @brief Helpers for Json objects
 */
namespace Cutter {

inline RVA jsonValueToRVA(const QJsonValue &value, RVA defaultValue = RVA_INVALID)
{
    bool ok;
    const RVA ret = value.toVariant().toULongLong(&ok);
    if (!ok) {
        return defaultValue;
    }
    return ret;
}

QTreeWidgetItem *jsonTreeWidgetItem(const QString &key, const CutterJson &json);

}

#endif // JSON_H
