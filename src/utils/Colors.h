#ifndef COLORS_H
#define COLORS_H

#include "cutter.h"
#include "libr/r_anal.h"

class Colors
{
public:
    Colors();
    static QString getColor(ut64 type);
};

#endif // COLORS_H
