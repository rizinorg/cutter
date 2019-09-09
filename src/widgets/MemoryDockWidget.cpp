#include "MemoryDockWidget.h"
#include "common/CutterSeekable.h"
#include "MainWindow.h"
#include <QAction>
#include <QEvent>

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

bool MemoryDockWidget::eventFilter(QObject *object, QEvent *event)
{
    if (mainWindow && event->type() == QEvent::FocusIn) {
        mainWindow->setCurrentMemoryWidget(this);
    }
    return CutterDockWidget::eventFilter(object, event);
}

void MemoryDockWidget::updateWindowTitle()
{
    QString name = getWindowTitle();
    QString id = getDockNumber();
    if (!id.isEmpty()) {
        name += " " + id;
    }
    if (!seekable->isSynchronized()) {
        name += CutterSeekable::tr(" (unsynced)");
    }
    setWindowTitle(name);
}

CutterSeekable* MemoryDockWidget::getSeekable() const
{
    return seekable;
}
