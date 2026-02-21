#ifndef DISASSEMBLYPREVIEW_H
#define DISASSEMBLYPREVIEW_H

#include <QTextBlockUserData>
#include "core/CutterDescriptions.h"

class QWidget;

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
bool showDebugValueTooltip(QWidget *parent, const QPoint &pointOfEvent, const QString &selectedText,
                           const RVA offset);

}
#endif
