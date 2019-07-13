#include "MemoryDockWidget.h"
#include "common/CutterSeekable.h"
#include "MainWindow.h"
#include <QAction>

MemoryDockWidget::MemoryDockWidget(CutterCore::MemoryWidgetType type, MainWindow *parent, QAction *action)
    : CutterDockWidget(parent, action)
    , mType(type), seekable(new CutterSeekable(this))
{
    //connect(Core(), &CutterCore::raisePrioritizedMemoryWidget, this, &MemoryDockWidget::handleRaiseMemoryWidget); //TODO:[#1616] cleanup this
    if (parent) {
        parent->addMemoryDockWidget(this);
    }
    connect(seekable, &CutterSeekable::syncChanged, this, &MemoryDockWidget::updateWindowTitle);
}

bool MemoryDockWidget::tryRaiseMemoryWidget()
{
    if (!seekable->isSynchronized()) {
        return false;
    }

    if (mType == CutterCore::MemoryWidgetType::Graph && Core()->isGraphEmpty()) {
        return false;
    }

    if (getBoundAction()) {
        getBoundAction()->setChecked(true);
    }

    show();
    raise();
    widgetToFocusOnRaise()->setFocus(Qt::FocusReason::TabFocusReason);
    return true;
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
