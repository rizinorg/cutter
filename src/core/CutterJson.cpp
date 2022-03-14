#include "core/CutterJson.h"

CutterJson CutterJson::last() const
{
    if (!has_children()) {
        return CutterJson();
    }

    const RzJson *last = value->children.first;
    while (last->next) {
        last = last->next;
    }

    return CutterJson(last, owner);
}

QStringList CutterJson::keys() const
{
    QStringList list;

    if (value && value->type == RZ_JSON_OBJECT) {
        for (const RzJson *child = value->children.first; child; child = child->next) {
            list.append(child->key);
        }
    }

    return list;
}
