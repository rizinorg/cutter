#include "CutterDockWidget.h"
#include "MainWindow.h"
#include "WidgetShortcuts.h"


CutterDockWidget::CutterDockWidget(MainWindow *main, QDockWidget *dockWidget, QAction *action) :
    QObject(main),
    dockWidget(dockWidget),
    action(action)
{
    dockWidget->installEventFilter(this);

    if (!action)
        return;

    main->addToDockWidgetList(this);
    main->addDockWidgetAction(this, action);
    connect(action, &QAction::triggered, this, &CutterDockWidget::toggleDockWidget);

    QKeySequence keySequence = widgetShortcuts[dockWidget->objectName()];
    if (keySequence.isEmpty())
        return;
    auto *toggleShortcut = new QShortcut(keySequence, main);
    connect(toggleShortcut, &QShortcut::activated, this, [=] () {
            toggleDockWidget(true);
            action->setChecked(dockWidget->isHidden());
    } );
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
