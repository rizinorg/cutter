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

/**
 * @brief Namespace to define relevant functions
 *
 * @ingroup DisassemblyPreview
 */
namespace DisassemblyPreview {
/*!
 * @brief Get the QString that defines the stylesheet for tooltip
 * @return A QString for the stylesheet
 */
QString getToolTipStyleSheet();

/*!
 * @brief Show a QToolTip that previews the disassembly that is pointed to
 * @return True if the tooltip is shown
 */
bool showDisasPreview(QWidget *parent, const QPoint &pointOfEvent, const RVA offsetFrom);
}
#endif
