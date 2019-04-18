#include "MemoryDockWidget.h"

#include <QAction>

MemoryDockWidget::MemoryDockWidget(CutterCore::MemoryWidgetType type, MainWindow *parent, QAction *action)
    : CutterDockWidget(parent, action)
    , mType(type)
{
    connect(Core(), &CutterCore::raisePrioritizedMemoryWidget, this, &MemoryDockWidget::handleRaiseMemoryWidget);
}

void MemoryDockWidget::handleRaiseMemoryWidget(CutterCore::MemoryWidgetType raiseType)
{
    bool raisingEmptyGraph = (raiseType == CutterCore::MemoryWidgetType::Graph && Core()->isGraphEmpty());
    if (raisingEmptyGraph) {
        raiseType = CutterCore::MemoryWidgetType::Disassembly;
    }

    if (raiseType == mType) {
        if (getBoundAction()) {
            getBoundAction()->setChecked(true);
        }

        show();
        raise();
        widgetToFocusOnRaise()->setFocus(Qt::FocusReason::TabFocusReason);
    }
}
