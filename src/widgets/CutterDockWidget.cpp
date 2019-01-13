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
    updateIsVisibleToUser();
}

CutterDockWidget::~CutterDockWidget() = default;

bool CutterDockWidget::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::FocusIn
        || event->type() == QEvent::ZOrderChange
        || event->type() == QEvent::Paint
        || event->type() == QEvent::Close
        || event->type() == QEvent::Show
        || event->type() == QEvent::Hide) {
        updateIsVisibleToUser();
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
    }
}

void CutterDockWidget::updateIsVisibleToUser()
{
    // Check if the user can actually see the widget.
    bool visibleToUser = isVisible() && !visibleRegion().isEmpty();
    if (visibleToUser == isVisibleToUserCurrent) {
        return;
    }
    isVisibleToUserCurrent = visibleToUser;
    if (isVisibleToUserCurrent) {
        emit becameVisibleToUser();
    }
}

void CutterDockWidget::closeEvent(QCloseEvent *event)
{
    if (action) {
        this->action->setChecked(false);
    }
    QDockWidget::closeEvent(event);
}

