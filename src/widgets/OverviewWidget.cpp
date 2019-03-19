#include "core/MainWindow.h"
#include "OverviewWidget.h"
#include "OverviewView.h"

OverviewWidget::OverviewWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action)
{
    setWindowTitle("Graph Overview");
    setObjectName("Graph Overview");
    setAllowedAreas(Qt::AllDockWidgetAreas);
    graphView = new OverviewView(this);
    setWidget(graphView);
    refreshDeferrer = createRefreshDeferrer([this]() {
        updateContents();
    });

    connect(this, &QDockWidget::visibilityChanged, this, [ = ](bool visibility) {
        if (visibility) {
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

void OverviewWidget::closeEvent(QCloseEvent *event)
{
    CutterDockWidget::closeEvent(event);
    emit graphClose();
}
