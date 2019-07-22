#include "core/MainWindow.h"
#include "OverviewWidget.h"
#include "GraphWidget.h"
#include "OverviewView.h"

OverviewWidget::OverviewWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action)
{
    setWindowTitle("Graph Overview");
    setObjectName("Graph Overview");
    setAllowedAreas(Qt::AllDockWidgetAreas);
    graphView = new OverviewView(this);
    setWidget(graphView);
    targetGraphWidget = nullptr;

    connect(graphView, SIGNAL(mouseMoved()), this, SLOT(updateTargetView()));

    graphDataRefreshDeferrer = createRefreshDeferrer([this]() {
        updateGraphData();
    });

    // Zoom shortcuts
    QShortcut *shortcut_zoom_in = new QShortcut(QKeySequence(Qt::Key_Plus), this);
    shortcut_zoom_in->setContext(Qt::WidgetWithChildrenShortcut);
    connect(shortcut_zoom_in, &QShortcut::activated, this, [this](){ zoomTarget(1); });
    QShortcut *shortcut_zoom_out = new QShortcut(QKeySequence(Qt::Key_Minus), this);
    shortcut_zoom_out->setContext(Qt::WidgetWithChildrenShortcut);
    connect(shortcut_zoom_out, &QShortcut::activated, this, [this](){ zoomTarget(-1); });
}

OverviewWidget::~OverviewWidget() {}

void OverviewWidget::resizeEvent(QResizeEvent *event)
{
    graphView->refreshView();
    updateRangeRect();
    QDockWidget::resizeEvent(event);
    emit resized();
}

void OverviewWidget::showEvent(QShowEvent *event)
{
    CutterDockWidget::showEvent(event);
    setUserOpened(true);
}

void OverviewWidget::closeEvent(QCloseEvent *event)
{
    CutterDockWidget::closeEvent(event);
    setUserOpened(false);
}

void OverviewWidget::setIsAvailable(bool isAvailable)
{
    if (this->isAvailable == isAvailable) {
        return;
    }
    this->isAvailable = isAvailable;
    if(!isAvailable) {
        hide();
    } else if(userOpened) {
        show();
    }
    emit isAvailableChanged(isAvailable);
}

void OverviewWidget::setUserOpened(bool userOpened)
{
    if (this->userOpened == userOpened) {
        return;
    }
    this->userOpened = userOpened;
    emit userOpenedChanged(userOpened);
}

void OverviewWidget::zoomTarget(int d)
{
    if (!targetGraphWidget) {
        return;
    }
    targetGraphWidget->getGraphView()->zoom(QPointF(0.5, 0.5), d);
}

void OverviewWidget::setTargetGraphWidget(GraphWidget *widget)
{
    if (widget == targetGraphWidget) {
        return;
    }
    if (targetGraphWidget) {
        disconnect(targetGraphWidget->getGraphView(), &DisassemblerGraphView::viewRefreshed, this, &OverviewWidget::updateGraphData);
        disconnect(targetGraphWidget->getGraphView(), &DisassemblerGraphView::resized, this, &OverviewWidget::updateRangeRect);
        disconnect(targetGraphWidget->getGraphView(), &GraphView::viewOffsetChanged, this, &OverviewWidget::updateRangeRect);
        disconnect(targetGraphWidget->getGraphView(), &GraphView::viewScaleChanged, this, &OverviewWidget::updateRangeRect);
        disconnect(targetGraphWidget, &GraphWidget::graphClosed, this, &OverviewWidget::targetClosed);
    }
    targetGraphWidget = widget;
    if (targetGraphWidget) {
        connect(targetGraphWidget->getGraphView(), &DisassemblerGraphView::viewRefreshed, this, &OverviewWidget::updateGraphData);
        connect(targetGraphWidget->getGraphView(), &DisassemblerGraphView::resized, this, &OverviewWidget::updateRangeRect);
        connect(targetGraphWidget->getGraphView(), &GraphView::viewOffsetChanged, this, &OverviewWidget::updateRangeRect);
        connect(targetGraphWidget->getGraphView(), &GraphView::viewScaleChanged, this, &OverviewWidget::updateRangeRect);
        connect(targetGraphWidget, &GraphWidget::graphClosed, this, &OverviewWidget::targetClosed);
    }
    updateGraphData();
    updateRangeRect();
    setIsAvailable(targetGraphWidget != nullptr);
}

void OverviewWidget::wheelEvent(QWheelEvent *event)
{
    zoomTarget(event->angleDelta().y() / 90);
    graphView->centreRect();
}

void OverviewWidget::targetClosed()
{
    setTargetGraphWidget(nullptr);
}

void OverviewWidget::updateTargetView()
{
    if (!targetGraphWidget) {
        return;
    }
    qreal curScale = graphView->getViewScale();
    int rectx = graphView->getRangeRect().x();
    int recty = graphView->getRangeRect().y();
    int overview_offset_x = graphView->getViewOffset().x();
    int overview_offset_y = graphView->getViewOffset().y();
    QPoint newOffset;
    newOffset.rx() = rectx / curScale + overview_offset_x;
    newOffset.ry() = recty / curScale + overview_offset_y;
    targetGraphWidget->getGraphView()->setViewOffset(newOffset);
    targetGraphWidget->getGraphView()->viewport()->update();
}

void OverviewWidget::updateGraphData()
{
    if (!graphDataRefreshDeferrer->attemptRefresh(nullptr)) {
        return;
    }
    if (targetGraphWidget && !targetGraphWidget->getGraphView()->isGraphEmpty()) {
        graphView->currentFcnAddr = targetGraphWidget->getGraphView()->currentFcnAddr;
        auto &mainGraphView = *targetGraphWidget->getGraphView();
        graphView->setData(mainGraphView.getWidth(), mainGraphView.getHeight(),
            mainGraphView.getBlocks(), mainGraphView.getEdgeConfigurations());
    } else {
        graphView->currentFcnAddr = RVA_INVALID;
        graphView->setData(0, 0, {}, {});
        graphView->setRangeRect(QRectF(0, 0, 0, 0));
    }
}

void OverviewWidget::updateRangeRect() {
    if (targetGraphWidget) {
        qreal curScale = graphView->getViewScale();
        qreal baseScale = targetGraphWidget->getGraphView()->getViewScale();
        qreal w = targetGraphWidget->getGraphView()->viewport()->width() * curScale / baseScale;
        qreal h = targetGraphWidget->getGraphView()->viewport()->height() * curScale / baseScale;
        int graph_offset_x = targetGraphWidget->getGraphView()->getViewOffset().x();
        int graph_offset_y = targetGraphWidget->getGraphView()->getViewOffset().y();
        int overview_offset_x = graphView->getViewOffset().x();
        int overview_offset_y = graphView->getViewOffset().y();
        int rangeRectX = graph_offset_x * curScale - overview_offset_x * curScale;
        int rangeRectY = graph_offset_y * curScale - overview_offset_y * curScale;
        graphView->setRangeRect(QRectF(rangeRectX, rangeRectY, w, h));
    } else {
        graphView->setRangeRect(QRectF(0, 0, 0, 0));
    }
}
