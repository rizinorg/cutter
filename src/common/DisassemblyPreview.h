#ifndef DISASSEMBLYPREVIEW_H
#define DISASSEMBLYPREVIEW_H

#include <QTextBlockUserData>

#include "core/CutterDescriptions.h"

class QWidget;

class DisassemblyTextBlockUserData : public QTextBlockUserData
{
public:
    DisassemblyLine line;

    explicit DisassemblyTextBlockUserData(const DisassemblyLine &line);
};

DisassemblyTextBlockUserData *getUserData(const QTextBlock &block);

namespace DisassemblyPreview
{
    QString getToolTipStyleSheet();
}
#endif
