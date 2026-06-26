#include "Colors.h"

#include "common/Configuration.h"

#include <utility>

void Colors::colorizeAssembly(RichTextPainter::List &list, QString opcode, ut64 typeNum)
{
    RichTextPainter::CustomRichText assembly;
    assembly.highlight = false;
    assembly.flags = RichTextPainter::FlagColor;

    // TODO cut opcode and use op["ptr"] to colorate registers and immediate values
    assembly.text = std::move(opcode);

    const QString colorName = Core()->getColorNameFromOp(typeNum);
    assembly.textColor = ConfigColor(colorName);
    list.push_back(assembly);
}

QColor Colors::overlayColor(const QColor &base, const QColor &overlay)
{
    const int alpha = overlay.alpha();
    if (alpha == 255) {
        return overlay;
    }
    if (alpha == 0) {
        return base;
    }

    const int r = (overlay.red() * alpha + base.red() * (255 - alpha)) / 255;
    const int g = (overlay.green() * alpha + base.green() * (255 - alpha)) / 255;
    const int b = (overlay.blue() * alpha + base.blue() * (255 - alpha)) / 255;

    return QColor(r, g, b);
}
