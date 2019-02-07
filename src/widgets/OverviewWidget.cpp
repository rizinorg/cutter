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
    refreshDeferrer = createRefreshDeferrer([this]() {
        updateContents();
    });

    connect(this, &QDockWidget::visibilityChanged, this, [ = ](bool visibility) {
        if (visibility) {
            Core()->setMemoryWidgetPriority(CutterCore::MemoryWidgetType::Graph);
            updateContents();
        }
    });
}

OverviewWidget::~OverviewWidget() {}

void OverviewWidget::resizeEvent(QResizeEvent *event)
{
    graphView->refreshView();
    QDockWidget::resizeEvent(event);
    emit resized();
}

void OverviewWidget::updateContents()
{
    if (!refreshDeferrer->attemptRefresh(nullptr)) {
        return;
    }
    graphView->refreshView();
}
