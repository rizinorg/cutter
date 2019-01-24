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
        connect(graphView->horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(adjustOverview()));
        connect(graphView->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(adjustOverview()));
        connect(overviewWidget->graphView, SIGNAL(mouseMoved()), this, SLOT(adjustGraph()));
    } else {
        disconnect(graphView, SIGNAL(refreshBlock()), this, SLOT(adjustOverview()));
        disconnect(graphView->horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(adjustOverview()));
        disconnect(graphView->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(adjustOverview()));
        disconnect(overviewWidget->graphView, SIGNAL(mouseMoved()), this, SLOT(adjustGraph()));
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
    bool scrollXVisible = graphView->unscrolled_render_offset_x == 0;
    bool scrollYVisible = graphView->unscrolled_render_offset_y == 0;
    if (!scrollXVisible && !scrollYVisible) {
        disableOverviewRect();
        return;
    }
    qreal x = 0;
    qreal y = 0;
    qreal w = overviewWidget->graphView->viewport()->width();
    qreal h = overviewWidget->graphView->viewport()->height();
    qreal curScale = overviewWidget->graphView->current_scale;
    qreal xoff = overviewWidget->graphView->unscrolled_render_offset_x;;
    qreal yoff = overviewWidget->graphView->unscrolled_render_offset_y;;

    w = graphView->viewport()->width();
    h = graphView->viewport()->height();

    if (scrollXVisible) {
        x = graphView->horizontalScrollBar()->value();
        w *= curScale;
    } else {
        xoff = 0;
    }

    if (scrollYVisible) {
        y = graphView->verticalScrollBar()->value();
        h *= curScale;
    } else {
        yoff = 0;
    }
    x *= curScale;
    y *= curScale;
    overviewWidget->graphView->rangeRect = QRectF(x + xoff, y + yoff, w, h);
    overviewWidget->graphView->viewport()->update();
}

void GraphWidget::adjustGraph()
{
    if (!overviewWidget) {
        return;
    }
    int x1 = overviewWidget->graphView->horizontalScrollBar()->value();
    int y1 = overviewWidget->graphView->verticalScrollBar()->value();
    qreal x2 = (overviewWidget->graphView->rangeRect.x() - (qreal)overviewWidget->graphView->unscrolled_render_offset_x)/ overviewWidget->graphView->current_scale;
    qreal y2 = (overviewWidget->graphView->rangeRect.y() - (qreal)overviewWidget->graphView->unscrolled_render_offset_y)/ overviewWidget->graphView->current_scale;

    graphView->horizontalScrollBar()->setValue(x1 + x2);
    graphView->verticalScrollBar()->setValue(y1 + y2);
}
