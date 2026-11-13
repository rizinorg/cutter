#include "RizinCpp.h"

#include <QSet>

QSet<QString> convertRzSetS(const RzSetS *set)
{
    if (!set) {
        return {};
    }

    RzIterator rzIt = {};
    rz_set_s_as_iter(set, &rzIt);

    QSet<QString> res;
    for (auto it = CutterRzIter<const char *>(rzIt); it; ++it) {
        res << QString::fromUtf8(*it);
    }
    return res;
}
