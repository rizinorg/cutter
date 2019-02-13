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
        connect(graphView->horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(adjustOverview()));
        connect(graphView->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(adjustOverview()));
        connect(overviewWidget->graphView, SIGNAL(refreshBlock()), this, SLOT(adjustOffset()));
        connect(overviewWidget->graphView, SIGNAL(mouseMoved()), this, SLOT(adjustGraph()));
        connect(overviewWidget, &QDockWidget::dockLocationChanged, this, &GraphWidget::adjustOverview);
        connect(overviewWidget, &OverviewWidget::resized, this, &GraphWidget::adjustOverview);
    } else {
        disconnect(graphView, SIGNAL(refreshBlock()), this, SLOT(adjustOverview()));
        disconnect(graphView, SIGNAL(viewZoomed()), this, SLOT(adjustOverview()));
        disconnect(graphView->horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(adjustOverview()));
        disconnect(graphView->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(adjustOverview()));
        disconnect(overviewWidget->graphView, SIGNAL(refreshBlock()), this, SLOT(adjustOffset()));
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
    overviewWidget->graphView->h_offset = 0;
    overviewWidget->graphView->v_offset = 0;
    bool scrollXVisible = graphView->horizontalScrollBar()->isVisible();
    bool scrollYVisible = graphView->verticalScrollBar()->isVisible();
    if (!scrollXVisible && !scrollYVisible) {
        disableOverviewRect();
        return;
    }
    qreal x = 0;
    qreal y = 0;
    qreal w = graphView->viewport()->width();
    qreal h = graphView->viewport()->height();
    qreal curScale = overviewWidget->graphView->current_scale;
    qreal baseScale = graphView->current_scale;

    w *= curScale / baseScale;
    h *= curScale / baseScale;
    if (scrollXVisible) {
        x = graphView->horizontalScrollBar()->value();
        x *= curScale;
    } else {
        x = (overviewWidget->graphView->viewport()->width() - w) / 2;
        overviewWidget->graphView->h_offset = x;
    }

    if (scrollYVisible) {
        y = graphView->verticalScrollBar()->value();
        y *= curScale;
    } else {
        y = (overviewWidget->graphView->viewport()->height() - h) / 2;
        overviewWidget->graphView->v_offset = y;
    }

    overviewWidget->graphView->rangeRect = QRectF(x + overviewWidget->graphView->unscrolled_render_offset_x,
            y + overviewWidget->graphView->unscrolled_render_offset_y, w, h);
    overviewWidget->graphView->viewport()->update();
}

void GraphWidget::adjustGraph()
{
    if (!overviewWidget) {
        return;
    }
    int x1 = overviewWidget->graphView->horizontalScrollBar()->value();
    int y1 = overviewWidget->graphView->verticalScrollBar()->value();
    qreal x2 = (overviewWidget->graphView->rangeRect.x() - (qreal)overviewWidget->graphView->unscrolled_render_offset_x) / overviewWidget->graphView->current_scale;
    qreal y2 = (overviewWidget->graphView->rangeRect.y() - (qreal)overviewWidget->graphView->unscrolled_render_offset_y) / overviewWidget->graphView->current_scale;

    graphView->horizontalScrollBar()->setValue(x1 + x2);
    graphView->verticalScrollBar()->setValue(y1 + y2);
}

void GraphWidget::adjustOffset()
{
    bool scrollXVisible = graphView->horizontalScrollBar()->isVisible();
    bool scrollYVisible = graphView->verticalScrollBar()->isVisible();
    if (!scrollXVisible) {
        overviewWidget->graphView->unscrolled_render_offset_x = 0;
    }
    if (!scrollYVisible) {
        overviewWidget->graphView->unscrolled_render_offset_y = 0;
    }
}
