#include "MemoryDockWidget.h"
#include "common/CutterSeekable.h"

#include <QAction>

MemoryDockWidget::MemoryDockWidget(CutterCore::MemoryWidgetType type, MainWindow *parent, QAction *action)
    : CutterDockWidget(parent, action)
    , mType(type), seekable(new CutterSeekable(this))
{
    connect(Core(), &CutterCore::raisePrioritizedMemoryWidget, this, &MemoryDockWidget::handleRaiseMemoryWidget);
    connect(seekable, &CutterSeekable::syncChanged, this, &MemoryDockWidget::updateWindowTitle);
}

void MemoryDockWidget::handleRaiseMemoryWidget(CutterCore::MemoryWidgetType raiseType)
{
    if (!seekable->isSynchronized()) {
        return;
    }
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

void MemoryDockWidget::updateWindowTitle()
{
    if (seekable->isSynchronized()) {
        setWindowTitle(getWindowTitle());
    } else {
        setWindowTitle(getWindowTitle() + CutterSeekable::tr(" (unsynced)"));
    }
}

CutterSeekable* MemoryDockWidget::getSeekable() const
{
    return seekable;
}
