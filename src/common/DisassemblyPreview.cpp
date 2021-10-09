#include "DisassemblyPreview.h"
#include "Configuration.h"

#include <QWidget>

DisassemblyTextBlockUserData::DisassemblyTextBlockUserData(const DisassemblyLine &line)
    : line { line }
{
}

DisassemblyTextBlockUserData *getUserData(const QTextBlock &block)
{
    QTextBlockUserData *userData = block.userData();
    if (!userData) {
        return nullptr;
    }

    return static_cast<DisassemblyTextBlockUserData *>(userData);
}

QString DisassemblyPreview::getToolTipStyleSheet()
{
    return QString{"QToolTip { border-width: 1px; max-width: %1px;"
                             "opacity: 230; background-color: %2;"
                             "color: %3; border-color: %3;}"}
                             .arg(400)
                             .arg(Config()->getColor("gui.tooltip.background").name())
                             .arg(Config()->getColor("gui.tooltip.foreground").name());
}
