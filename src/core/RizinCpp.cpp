#include "RizinCpp.h"

#include <QSet>

QSet<QString> convertRzSetS(const RzSetS *set)
{
    QSet<QString> res;
    for (auto it = CutterRzIter<const char *>(rz_set_s_as_iter(set)); it; ++it) {
        res << QString::fromUtf8(*it);
    }
    return res;
}
