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

void CutterDockWidget::closeEvent(QCloseEvent *event)
{
    if (action) {
        this->action->setChecked(false);
    }
    QDockWidget::closeEvent(event);
}

CutterDockWidget::~CutterDockWidget() {}
