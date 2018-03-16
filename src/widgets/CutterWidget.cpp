#include "CutterWidget.h"
#include "MainWindow.h"


CutterWidget::CutterWidget(MainWindow *main, QAction *action) :
    QDockWidget(main),
    action(action)
{
    main->addToDockWidgetList(this);
    if (action) {
        main->addDockWidgetAction(this, action);
        connect(action, &QAction::triggered, this, &CutterWidget::toggleDockWidget);
    }
}


void CutterWidget::toggleDockWidget(bool show)
{
    if (!show)
    {
        this->close();
    }
    else
    {
        this->show();
        this->raise();
    }
}

void CutterWidget::closeEvent(QCloseEvent * event) {
    if (action) {
        this->action->setChecked(false);
    }
    QDockWidget::closeEvent(event);
}

CutterWidget::~CutterWidget() {}
