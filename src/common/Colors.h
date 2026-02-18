#ifndef COLORS_H
#define COLORS_H

#include "core/Cutter.h"
#include "common/RichTextPainter.h"
#include <rz_analysis.h>

class Colors
{
public:
    Colors();
    static void colorizeAssembly(RichTextPainter::List &list, QString opcode, ut64 type_num);
    static QString getColor(ut64 type);
    /**
     * @brief Blends an overlay color onto an existing base color
     * @param base The original color (assumes base color is solid)
     * @param overlay The overlay color with alpha transparency
     * @return The resulting blended color
     */
    static QColor overlayColor(const QColor &base, const QColor &overlay);
};

#endif // COLORS_H
