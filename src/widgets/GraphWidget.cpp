#include "MainWindow.h"
#include "GraphWidget.h"
#include "DisassemblerGraphView.h"

GraphWidget::GraphWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action)
{
    this->setObjectName("Graph");
    this->setAllowedAreas(Qt::AllDockWidgetAreas);
    this->graphView = new DisassemblerGraphView(this);
    this->setWidget(graphView);

    connect(this, &QDockWidget::visibilityChanged, this, [](bool visibility) {
        if (visibility) {
            Core()->setMemoryWidgetPriority(CutterCore::MemoryWidgetType::Graph);
        }
    });

    connect(Core(), &CutterCore::raisePrioritizedMemoryWidget,
    this, [ = ](CutterCore::MemoryWidgetType type) {
        if (type == CutterCore::MemoryWidgetType::Graph) {
            this->raise();
            this->graphView->setFocus();
        }
    });
}

GraphWidget::~GraphWidget() {}
