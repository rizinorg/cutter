#ifndef COLORS_H
#define COLORS_H

#include "common/RichTextPainter.h"
#include "core/Cutter.h"

/**
 * @brief Utilities for handling color logic and syntax highlighting for assembly
 */
namespace Colors {

/**
 * @brief Colorize asssembly text
 * @param list RichTextPainter::List to add the opcode to
 * @param opcode Opcode to colorize
 * @param typeNum Type of opcode
 */
void colorizeAssembly(RichTextPainter::List &list, QString opcode, ut64 typeNum);

/**
 * @brief Blends an overlay color onto an existing base color
 * @param base The original color (assumes base color is solid)
 * @param overlay The overlay color with alpha transparency
 * @return The resulting blended color
 */
QColor overlayColor(const QColor &base, const QColor &overlay);
}

#endif // COLORS_H
