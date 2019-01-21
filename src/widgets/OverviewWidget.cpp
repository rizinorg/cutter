#include "MainWindow.h"
#include "OverviewWidget.h"
#include "OverviewView.h"

OverviewWidget::OverviewWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action)
{
    this->setWindowTitle("Graph Overview");
    this->setObjectName("Graph Overview");
    this->setAllowedAreas(Qt::AllDockWidgetAreas);
    this->graphView = new OverviewView(this);
    this->setWidget(graphView);

    connect(this, &QDockWidget::visibilityChanged, this, [ = ](bool visibility) {
        if (visibility) {
            Core()->setMemoryWidgetPriority(CutterCore::MemoryWidgetType::Graph);
            graphView->refreshView();
        }
    });
}

OverviewWidget::~OverviewWidget() {}

void OverviewWidget::resizeEvent(QResizeEvent *event)
{
    graphView->refreshView();
    QDockWidget::resizeEvent(event);
}
