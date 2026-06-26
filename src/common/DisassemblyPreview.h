#ifndef DISASSEMBLYPREVIEW_H
#define DISASSEMBLYPREVIEW_H

#include "DisassemblyHelper.h"
#include "core/CutterDescriptions.h"

#include <QTextBlockUserData>

class QWidget;

/**
 * @namespace Namespace containing functions for showing tooltips in disassembly
 */
namespace DisassemblyPreview {
/*!
 * @brief Get the QString that defines the stylesheet for tooltip
 * @return A QString for the stylesheet
 */
QString getToolTipStyleSheet();

/*!
 * @brief Show a QToolTip that previews the disassembly that is pointed to
 * It works for GraphWidget and DisassemblyWidget
 * @return True if the tooltip is shown
 */
bool showDisasPreview(QWidget *parent, const QPoint &pointOfEvent, const RVA offsetFromk);

/**
 * @brief Show a QToolTip that previews the disassembly at a specific address
 * @return True if the tooltip is shown
 */
bool showDisasPreviewAt(QWidget *parent, const QPoint &pointOfEvent, const RVA offset);

/**
 * @brief Show a QToolTip that shows the value of the highlighted register, variable, or memory
 * @return True if the tooltip is shown
 */
bool showDebugValueTooltip(QWidget *parent, const QPoint &pointOfEvent,
                           const DisassemblyHelper::TargetAction &ta,
                           const DisassemblyHelper::TargetContext &ctx);

/**
 * @brief Displays a tooltip or preview window based on the item under the cursor
 * @param parent The widget that owns and positions the tooltip
 * @param globalPos The screen position where the tooltip should appear
 * @param ctx The context identifying the word and address under the mouse
 * @param hasPreview The config value indicating whether preview should be shown for the widget
 * @return True if a tooltip was shown or the event was handled, false otherwise
 */
bool showTooltip(QWidget *parent, const QPoint &globalPos,
                 const DisassemblyHelper::TargetContext &ctx, bool hasPreview);
}
#endif
