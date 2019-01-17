#include "MainWindow.h"
#include "OverviewWidget.h"
#include "OverviewView.h"
#include "WidgetShortcuts.h"

OverviewWidget::OverviewWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action)
{
    this->setObjectName("Graph");
    this->setAllowedAreas(Qt::AllDockWidgetAreas);
    this->graphView = new OverviewView(this);
    this->setWidget(graphView);

    QShortcut *toggle_shortcut = new QShortcut(widgetShortcuts["OverviewWidget"], main);
    connect(toggle_shortcut, &QShortcut::activated, this, [ = ]() {
            toggleDockWidget(true); 
            main->updateDockActionChecked(action);
    });

    connect(this, &QDockWidget::visibilityChanged, this, [ = ](bool visibility) {
        if (visibility) {
            Core()->setMemoryWidgetPriority(CutterCore::MemoryWidgetType::Graph);
            this->graphView->header->setFixedWidth(width());
        }
    });

    connect(Core(), &CutterCore::raisePrioritizedMemoryWidget,
    this, [ = ](CutterCore::MemoryWidgetType type) {
        bool emptyGraph = (type == CutterCore::MemoryWidgetType::Graph && Core()->isGraphEmpty());
        if (type == CutterCore::MemoryWidgetType::Graph && !emptyGraph) {
            this->raise();
            this->graphView->setFocus();
            this->graphView->header->setFixedWidth(width());
        }
    });
}

OverviewWidget::~OverviewWidget() {}

void OverviewWidget::resizeEvent(QResizeEvent *event)
{

    graphView->refreshView();
    QDockWidget::resizeEvent(event);
}
