#include "MainWindow.h"
#include "GraphWidget.h"
#include "DisassemblerGraphView.h"
#include "WidgetShortcuts.h"
#include "OverviewView.h"

GraphWidget::GraphWidget(MainWindow *main, OverviewWidget *overview, QAction *action) :
    CutterDockWidget(main, action)
{
    this->setObjectName("Graph");
    this->setAllowedAreas(Qt::AllDockWidgetAreas);
    this->graphView = new DisassemblerGraphView(this);
    this->setWidget(graphView);
    this->overviewWidget = overview;

    // getting the name of the class is implementation defined, and cannot be
    // used reliably across different compilers.
    //QShortcut *toggle_shortcut = new QShortcut(widgetShortcuts[typeid(this).name()], main);
    QShortcut *toggle_shortcut = new QShortcut(widgetShortcuts["GraphWidget"], main);
    connect(toggle_shortcut, &QShortcut::activated, this, [ = ]() {
            toggleDockWidget(true); 
            main->updateDockActionChecked(action);
    });

    connect(this, &QDockWidget::visibilityChanged, this, [ = ](bool visibility) {
        toggleOverview(visibility);
        if (visibility) {
            Core()->setMemoryWidgetPriority(CutterCore::MemoryWidgetType::Graph);
            this->graphView->header->setFixedWidth(width());
        }
    });

    connect(graphView, &DisassemblerGraphView::viewRefreshed, this, [ = ]() {
        overviewWidget->graphView->setData(graphView->getWidth(), graphView->getHeight(), graphView->getBlocks());
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

GraphWidget::~GraphWidget() {}

void GraphWidget::toggleOverview(bool visibility)
{
    if (!overviewWidget) {
        return;
    }
    if (visibility) {
        connect(graphView, SIGNAL(refreshBlock()), this, SLOT(adjustOverview()));
        connect(graphView, SIGNAL(viewZoomed()), this, SLOT(adjustOverview()));
        connect(overviewWidget->graphView, SIGNAL(mouseMoved()), this, SLOT(adjustGraph()));
        connect(overviewWidget, &QDockWidget::dockLocationChanged, this, &GraphWidget::adjustOverview);
        connect(overviewWidget, &OverviewWidget::resized, this, &GraphWidget::adjustOverview);
    } else {
        disconnect(graphView, SIGNAL(refreshBlock()), this, SLOT(adjustOverview()));
        disconnect(graphView, SIGNAL(viewZoomed()), this, SLOT(adjustOverview()));
        disconnect(overviewWidget->graphView, SIGNAL(mouseMoved()), this, SLOT(adjustGraph()));
        disconnect(overviewWidget, &QDockWidget::dockLocationChanged, this, &GraphWidget::adjustOverview);
        disconnect(overviewWidget, &OverviewWidget::resized, this, &GraphWidget::adjustOverview);
        disableOverviewRect();
    }
}

void GraphWidget::disableOverviewRect()
{
    if (!overviewWidget) {
        return;
    }
    overviewWidget->graphView->rangeRect = QRectF(0, 0, 0, 0);
    overviewWidget->graphView->viewport()->update();
}

void GraphWidget::adjustOverview()
{
    if (!overviewWidget) {
        return;
    }
    qreal curScale = overviewWidget->graphView->current_scale;
    qreal baseScale = graphView->current_scale;
    qreal w = graphView->viewport()->width() * curScale / baseScale;
    qreal h = graphView->viewport()->height() * curScale / baseScale;
    int graph_offset_x = graphView->offset_x;
    int graph_offset_y = graphView->offset_y;
    int overview_offset_x = overviewWidget->graphView->offset_x;
    int overview_offset_y = overviewWidget->graphView->offset_y;
    int rangeRectX = graph_offset_x * curScale - overview_offset_x;
    int rangeRectY = graph_offset_y * curScale - overview_offset_y;

    overviewWidget->graphView->rangeRect = QRectF(rangeRectX, rangeRectY, w, h);
    overviewWidget->graphView->viewport()->update();
}

void GraphWidget::adjustGraph()
{
    if (!overviewWidget) {
        return;
    }
    qreal curScale = overviewWidget->graphView->current_scale;
    qreal baseScale = graphView->current_scale;
    int rectx = overviewWidget->graphView->rangeRect.x();
    int recty = overviewWidget->graphView->rangeRect.y();
    int overview_offset_x = overviewWidget->graphView->offset_x;
    int overview_offset_y = overviewWidget->graphView->offset_y;
    graphView->offset_x = (rectx + overview_offset_x) / curScale;
    graphView->offset_y = (recty + overview_offset_y) / curScale;
    graphView->viewport()->update();
}
