#ifndef COLORS_H
#define COLORS_H

#include "Cutter.h"
#include "utils/RichTextPainter.h"
#include <r_anal.h>

class Colors
{
public:
    Colors();
    static void colorizeAssembly(RichTextPainter::List &list, QString opcode, ut64 type_num);
    static QString getColor(ut64 type);
};

#endif // COLORS_H
