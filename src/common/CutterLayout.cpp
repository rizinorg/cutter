#include "CutterLayout.h"

using namespace Cutter;

bool Cutter::isBuiltinLayoutName(const QString &name)
{
    return name == layoutDefault || name == layoutDebug;
}
