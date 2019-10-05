#include "CutterDockWidget.h"
#include "core/MainWindow.h"

#include <QAction>
#include <QEvent>
#include <QtWidgets/QShortcut>

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

    QShortcut *shortcut_zoom_in = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Plus), this);
    shortcut_zoom_in->setContext(Qt::WidgetWithChildrenShortcut);
    connect(shortcut_zoom_in, &QShortcut::activated, this, &CutterDockWidget::zoomIn);

    QShortcut *shortcut_zoom_out = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Minus), this);
    shortcut_zoom_out->setContext(Qt::WidgetWithChildrenShortcut);
    connect(shortcut_zoom_out, &QShortcut::activated, this, &CutterDockWidget::zoomOut);

    QShortcut *shortcut_zoom_reset = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Equal), this);
    shortcut_zoom_reset->setContext(Qt::WidgetWithChildrenShortcut);
    connect(shortcut_zoom_reset, &QShortcut::activated, this, &CutterDockWidget::zoomReset);
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

void CutterDockWidget::zoomIn()
{
  Config()->setZoomFactor(Config()->getZoomFactor() + 0.1);
}

void CutterDockWidget::zoomOut()
{
  Config()->setZoomFactor(Config()->getZoomFactor() - 0.1);
}

void CutterDockWidget::zoomReset()
{
  Config()->setZoomFactor(1.0);
}


