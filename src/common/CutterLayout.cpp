#include "CutterLayout.h"

using namespace Cutter;

bool Cutter::isBuiltinLayoutName(const QString &name)
{
    return name == LAYOUT_DEFAULT || name == LAYOUT_DEBUG;
}
