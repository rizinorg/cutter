
#ifndef CUTTER_JSON_H
#define CUTTER_JSON_H

#include "core/Cutter.h"

#include <QJsonValue>

static inline RVA JsonValueGetRVA(const QJsonValue &value, RVA defaultValue = RVA_INVALID)
{
    bool ok;
    RVA ret = value.toVariant().toULongLong(&ok);
    if (!ok) {
        return defaultValue;
    }
    return ret;
}

#endif //CUTTER_JSON_H
