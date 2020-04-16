#include "CutterDockWidget.h"
#include "core/MainWindow.h"

#include <QAction>
#include <QEvent>
#include <QSet>
#include <QtWidgets/QShortcut>

CutterDockWidget::CutterDockWidget(MainWindow *parent, QAction *action)
    : QDockWidget(parent), mainWindow(parent), action(action)
{
    if (action) {
        addAction(action);
        connect(action, &QAction::triggered, this,
                &CutterDockWidget::toggleDockWidget);
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
    if (event->type() == QEvent::FocusIn ||
            event->type() == QEvent::ZOrderChange || event->type() == QEvent::Paint ||
            event->type() == QEvent::Close || event->type() == QEvent::Show ||
            event->type() == QEvent::Hide) {
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

QWidget *CutterDockWidget::widgetToFocusOnRaise() { return this; }

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

QAction *CutterDockWidget::getBoundAction() const { return action; }

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

void CutterDockWidget::keyPressEvent(QKeyEvent *event)
{
    firstKeyRelease = true;
    // SegFault here, event uninitialised
    auto e = event;
    //keysPressed->insert(event->key());
}

void CutterDockWidget::keyReleaseEvent(QKeyEvent *event)
{
    if (firstKeyRelease) {
        processKeyShortcut();
        firstKeyRelease = false;
    }
    //keysPressed->remove(event->key());
}

void CutterDockWidget::processKeyShortcut()
{
    bool handled = false;
    if (keysPressed->contains(Qt::Key_Control)) {
        /*if (keysPressed->contains(Qt::Key_Tab)) {
          auto dockList = mainWindow->getListOfDockWidgets();
          QList<QDockWidget*>::iterator currentWidgetIt;
          for (auto i = dockList.begin(); i != dockList.end(); ++i)
          {
            if ((dynamic_cast<CutterDockWidget*>(*currentWidgetIt))->getDockNumber() == getDockNumber())
            {
              currentWidgetIt = i;
              break;
            }
          }
          CutterDockWidget* currentWidget = dynamic_cast<CutterDockWidget*>(*currentWidgetIt);
          if (keysPressed->contains(Qt::Key_Shift)) {
            // TODO: Previous open tab
            if (currentWidgetIt != dockList.begin())
            {
              CutterDockWidget* newWidget = dynamic_cast<CutterDockWidget*>(*currentWidgetIt - 1);
              currentWidget->toggleDockWidget(false);
              newWidget->toggleDockWidget(true);
            }
          } else {
            // TODO: Next open tab
            if (currentWidgetIt != dockList.end() -1)
            {
              CutterDockWidget* newWidget = dynamic_cast<CutterDockWidget*>(*currentWidgetIt + 1);
              currentWidget->toggleDockWidget(false);
              newWidget->toggleDockWidget(true);
            }
          }
        } else {
          if (!keysPressed->contains(Qt::Key_Shift)) {
            if (keysPressed->contains(Qt::Key_T)) {
              // TODO: Duplicate current tab
            } else if (keysPressed->contains(Qt::Key_W)) {
              // TODO: Close current tab
            }
          }
        }*/
    }
}
