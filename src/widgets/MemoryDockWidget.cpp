#include "MemoryDockWidget.h"
#include "common/CutterSeekable.h"
#include "MainWindow.h"
#include <QAction>

MemoryDockWidget::MemoryDockWidget(MemoryWidgetType type, MainWindow *parent, QAction *action)
    : CutterDockWidget(parent, action)
    , mType(type), seekable(new CutterSeekable(this))
{
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

    if (mType == MemoryWidgetType::Graph && Core()->isGraphEmpty()) {
        return false;
    }
    raiseMemoryWidget();

    return true;
}

void MemoryDockWidget::raiseMemoryWidget()
{

    if (getBoundAction()) {
        getBoundAction()->setChecked(true);
    }

    show();
    raise();
    widgetToFocusOnRaise()->setFocus(Qt::FocusReason::TabFocusReason);
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
