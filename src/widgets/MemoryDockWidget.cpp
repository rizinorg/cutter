#include "MemoryDockWidget.h"

#include "MainWindow.h"
#include "common/CutterSeekable.h"

#include <QAction>
#include <QContextMenuEvent>
#include <QEvent>
#include <QMenu>

MemoryDockWidget::MemoryDockWidget(MemoryWidgetType type, MainWindow *parent)
    : AddressableDockWidget(parent), type(type)
{
    if (parent) {
        parent->addMemoryDockWidget(this);
    }
}

bool MemoryDockWidget::tryRaiseMemoryWidget()
{
    if (!seekable->isSynchronized()) {
        return false;
    }

    if (type == MemoryWidgetType::Graph && Core()->isGraphEmpty()) {
        return false;
    }
    raiseMemoryWidget();

    return true;
}

bool MemoryDockWidget::eventFilter(QObject *object, QEvent *event)
{
    if (mainWindow && event->type() == QEvent::FocusIn) {
        mainWindow->setCurrentMemoryWidget(this);
    }
    return CutterDockWidget::eventFilter(object, event);
}
