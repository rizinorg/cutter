#include "MemoryDockWidget.h"
#include "common/CutterSeekable.h"

#include <QAction>

MemoryDockWidget::MemoryDockWidget(CutterCore::MemoryWidgetType type, MainWindow *parent, QAction *action)
    : CutterDockWidget(parent, action)
    , mType(type), seekable(new CutterSeekable(this))
{
    connect(Core(), &CutterCore::raisePrioritizedMemoryWidget, this, &MemoryDockWidget::handleRaiseMemoryWidget);
    connect(seekable, &CutterSeekable::syncChanged, this, [this]() {
        if (seekable->isSynchronized()) {
            setWindowTitle(windowTitle().remove(CutterSeekable::tr(" (unsynced)")));
        } else {
            setWindowTitle(windowTitle() + CutterSeekable::tr(" (unsynced)"));
        }
    });
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

CutterSeekable* MemoryDockWidget::getSeekable() const
{
    return seekable;
}
