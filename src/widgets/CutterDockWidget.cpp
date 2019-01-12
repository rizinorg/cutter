#include "CutterDockWidget.h"
#include "MainWindow.h"


CutterDockWidget::CutterDockWidget(MainWindow *main, QAction *action) :
    QDockWidget(main),
    action(action)
{
    if (action) {
        main->addToDockWidgetList(this);
        main->addDockWidgetAction(this, action);
        connect(action, &QAction::triggered, this, &CutterDockWidget::toggleDockWidget);
    }

    // Install event filter to catch redraw widgets when needed
    installEventFilter(this);
}

CutterDockWidget::~CutterDockWidget() {}

bool CutterDockWidget::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::FocusIn || event->type() == QEvent::Paint) {
        qDebug() << object << "is now focused in";
        refreshIfNeeded();
    }
    return QDockWidget::eventFilter(object, event);
}

void CutterDockWidget::toggleDockWidget(bool show)
{
    if (!show) {
        this->close();
    } else {
        this->show();
        this->raise();
        this->refreshIfNeeded();
    }
}

void CutterDockWidget::refreshIfNeeded()
{
    if (doRefresh) {
        refreshContent();
        doRefresh = false;
    }
}

void CutterDockWidget::closeEvent(QCloseEvent *event)
{
    if (action) {
        this->action->setChecked(false);
    }
    QDockWidget::closeEvent(event);
}

bool CutterDockWidget::isVisibleToUser()
{
    // Check if the user can actually see the widget.
    bool visibleToUser = this->isVisible() && !this->visibleRegion().isEmpty();
    if (!visibleToUser) {
        // If this is called and not visible, it must be refreshed later
        doRefresh = true;
    }
    return visibleToUser;
}
