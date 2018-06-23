#include "CutterDockWidget.h"
#include "MainWindow.h"


CutterDockWidget::CutterDockWidget(MainWindow *main, QDockWidget *dockWidget, QAction *action) :
    QObject(main),
    dockWidget(dockWidget),
    action(action)
{
    if (action) {
        main->addToDockWidgetList(this);
        main->addDockWidgetAction(this, action);
        connect(action, &QAction::triggered, this, &CutterDockWidget::toggleDockWidget);
    }

    dockWidget->installEventFilter(this);
}

CutterDockWidget::~CutterDockWidget() = default;

void CutterDockWidget::toggleDockWidget(bool show)
{
    if (!show) {
        dockWidget->close();
    } else {
        dockWidget->show();
        dockWidget->raise();
    }
}

bool CutterDockWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Close) {
        if (action) {
            this->action->setChecked(false);
        }
    }
    return QObject::eventFilter(obj, event);
}
