#include "OverviewWidget.h"

#include "GraphWidget.h"
#include "OverviewView.h"
#include "core/MainWindow.h"
#include "shortcuts/ShortcutManager.h"

OverviewWidget::OverviewWidget(MainWindow *main)
    : CutterDockWidget(main),
      graphView(new OverviewView(this)),
      targetGraphWidget(nullptr),
      graphDataRefreshDeferrer(createRefreshDeferrer([this]() { updateGraphData(); }))
{
    setWindowTitle(tr("Graph Overview"));
    setObjectName("Graph Overview");
    setAllowedAreas(Qt::AllDockWidgetAreas);

    setWidget(graphView);

    connect(graphView, &OverviewView::mouseMoved, this, &OverviewWidget::updateTargetView);

    // Zoom shortcuts
    QShortcut *shortcutZoomIn = Shortcuts()->makeQShortcut("Overview.zoomIn", this);
    shortcutZoomIn->setContext(Qt::WidgetWithChildrenShortcut);
    connect(shortcutZoomIn, &QShortcut::activated, this, [this]() { zoomTarget(1); });
    QShortcut *shortcutZoomOut = Shortcuts()->makeQShortcut("Overview.zoomOut", this);
    shortcutZoomOut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(shortcutZoomOut, &QShortcut::activated, this, [this]() { zoomTarget(-1); });
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
    if (!isAvailable) {
        hide();
    } else if (userOpened) {
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
        disconnect(targetGraphWidget->getGraphView(), &DisassemblerGraphView::viewRefreshed, this,
                   &OverviewWidget::updateGraphData);
        disconnect(targetGraphWidget->getGraphView(), &DisassemblerGraphView::resized, this,
                   &OverviewWidget::updateRangeRect);
        disconnect(targetGraphWidget->getGraphView(), &GraphView::viewOffsetChanged, this,
                   &OverviewWidget::updateRangeRect);
        disconnect(targetGraphWidget->getGraphView(), &GraphView::viewScaleChanged, this,
                   &OverviewWidget::updateRangeRect);
        disconnect(targetGraphWidget, &GraphWidget::graphClosed, this,
                   &OverviewWidget::targetClosed);
    }
    targetGraphWidget = widget;
    if (targetGraphWidget) {
        connect(targetGraphWidget->getGraphView(), &DisassemblerGraphView::viewRefreshed, this,
                &OverviewWidget::updateGraphData);
        connect(targetGraphWidget->getGraphView(), &DisassemblerGraphView::resized, this,
                &OverviewWidget::updateRangeRect);
        connect(targetGraphWidget->getGraphView(), &GraphView::viewOffsetChanged, this,
                &OverviewWidget::updateRangeRect);
        connect(targetGraphWidget->getGraphView(), &GraphView::viewScaleChanged, this,
                &OverviewWidget::updateRangeRect);
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
    const qreal curScale = graphView->getViewScale();
    const int rectx = graphView->getRangeRect().x();
    const int recty = graphView->getRangeRect().y();
    const int overviewOffsetX = graphView->getViewOffset().x();
    const int overviewOffsetY = graphView->getViewOffset().y();
    QPoint newOffset;
    newOffset.rx() = rectx / curScale + overviewOffsetX;
    newOffset.ry() = recty / curScale + overviewOffsetY;
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

void OverviewWidget::updateRangeRect()
{
    if (targetGraphWidget) {
        const qreal curScale = graphView->getViewScale();
        const qreal baseScale = targetGraphWidget->getGraphView()->getViewScale();
        const qreal w =
                targetGraphWidget->getGraphView()->viewport()->width() * curScale / baseScale;
        const qreal h =
                targetGraphWidget->getGraphView()->viewport()->height() * curScale / baseScale;
        const int graphOffsetX = targetGraphWidget->getGraphView()->getViewOffset().x();
        const int graphOffsetY = targetGraphWidget->getGraphView()->getViewOffset().y();
        const int overviewOffsetX = graphView->getViewOffset().x();
        const int overviewOffsetY = graphView->getViewOffset().y();
        const int rangeRectX = graphOffsetX * curScale - overviewOffsetX * curScale;
        const int rangeRectY = graphOffsetY * curScale - overviewOffsetY * curScale;
        graphView->setRangeRect(QRectF(rangeRectX, rangeRectY, w, h));
    } else {
        graphView->setRangeRect(QRectF(0, 0, 0, 0));
    }
}
