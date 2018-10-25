#include "MainWindow.h"
#include "GraphWidget.h"
#include "DisassemblerGraphView.h"

GraphWidget::GraphWidget(MainWindow *main) :
    QDockWidget(main)
{
    setObjectName(QStringLiteral("GraphWidget"));
    setWindowTitle(QStringLiteral("Graph"));
    setAllowedAreas(Qt::AllDockWidgetAreas);
    graphView = new DisassemblerGraphView(this);
    setWidget(graphView);

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

GraphWidget::~GraphWidget() {}
