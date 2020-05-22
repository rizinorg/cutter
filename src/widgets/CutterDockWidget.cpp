#include "CutterDockWidget.h"
#include "core/MainWindow.h"

#include <QEvent>
#include <QtWidgets/QShortcut>

CutterDockWidget::CutterDockWidget(MainWindow *parent, QAction *)
    : CutterDockWidget(parent)
{
}

CutterDockWidget::CutterDockWidget(MainWindow *parent) :
    QDockWidget(parent),
    mainWindow(parent)
{
    // Install event filter to catch redraw widgets when needed
    installEventFilter(this);
    updateIsVisibleToUser();
    connect(toggleViewAction(), &QAction::triggered, this, &QWidget::raise);
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

QVariantMap CutterDockWidget::serializeViewProprties()
{
    return {};
}

void CutterDockWidget::deserializeViewProperties(const QVariantMap &)
{
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
    QDockWidget::closeEvent(event);
    if (isTransient) {
        if (mainWindow) {
            mainWindow->removeWidget(this);
        }

        // remove parent, otherwise dock layout may still decide to use this widget which is about to be deleted
        setParent(nullptr);

        deleteLater();
    }

    emit closed();
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
