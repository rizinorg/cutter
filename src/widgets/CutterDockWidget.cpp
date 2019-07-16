#include "CutterDockWidget.h"
#include "core/MainWindow.h"

#include <QAction>
#include <QEvent>

CutterDockWidget::CutterDockWidget(MainWindow *parent, QAction *action) :
    QDockWidget(parent),
    mainWindow(parent),
    action(action)
{
    if (action) {
        addAction(action);
        connect(action, &QAction::triggered, this, &CutterDockWidget::toggleDockWidget);
    }
    if (parent) {
        parent->addWidget(this);
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
        this->hide();
    } else {
        this->show();
        this->raise();
    }
}

QWidget *CutterDockWidget::widgetToFocusOnRaise()
{
    return this;
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
    if (isTransient) {
        if (mainWindow) {
            mainWindow->removeWidget(this);
        }
        deleteLater();
    }

    emit closed();
}

QAction *CutterDockWidget::getBoundAction() const
{
    return action;
}

QString CutterDockWidget::getDockNumber()
{
    auto name = this->objectName();
    if (name.contains(';')) {
        auto parts = name.split(';');
        if (parts.size() >= 2) {
            return parts[1];
        }
    }
    return QString();
}

