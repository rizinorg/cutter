#include "MainWindow.h"
#include "GraphWidget.h"
#include "DisassemblerGraphView.h"

GraphWidget::GraphWidget(MainWindow *main, QAction *action)
    : CutterDockWidget(main, action)
{
    setObjectName("Graph");
    setAllowedAreas(Qt::AllDockWidgetAreas);
    graphView = new DisassemblerGraphView(this);
    headerLabel = new QLabel(this);
    headerLabel->setIndent(5);
    headerLabel->setAlignment(Qt::AlignTop);
    DisassemblyScrollArea *w = new DisassemblyScrollArea(this);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(graphView);
    layout->setMargin(0);
    w->viewport()->setLayout(layout);
    w->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setWidget(w);

    connect(this, &QDockWidget::visibilityChanged, this, [](bool visibility) {
        if (visibility) {
            Core()->setMemoryWidgetPriority(CutterCore::MemoryWidgetType::Graph);
        }
    });

    connect(Core(), &CutterCore::raisePrioritizedMemoryWidget,
    this, [ = ](CutterCore::MemoryWidgetType type) {
        bool emptyGraph = (type == CutterCore::MemoryWidgetType::Graph && Core()->isGraphEmpty());
        if (type == CutterCore::MemoryWidgetType::Graph && !emptyGraph) {
            this->raise();
            this->graphView->setFocus();
            QString afcf = Core()->cmd("afcf").trimmed();
            if (afcf.length() > 0) {
                layout->insertWidget(0, headerLabel);
                headerLabel->setText(Core()->cmd("afcf").trimmed());
                headerLabel->show();
            } else {
                layout->removeWidget(headerLabel);
                headerLabel->hide();
            }
        }
    });

    connect(Core(), &CutterCore::raisePrioritizedMemoryWidget,
    this, [ = ](CutterCore::MemoryWidgetType type) {
        bool emptyGraph = (type == CutterCore::MemoryWidgetType::Graph && Core()->isGraphEmpty());
        if (type == CutterCore::MemoryWidgetType::Graph && !emptyGraph) {
            this->raise();
            this->graphView->setFocus();
            QString afcf = Core()->cmd("afcf").trimmed();
            if (afcf.length() > 0) {
                layout->insertWidget(0, headerLabel);
                headerLabel->setText(Core()->cmd("afcf").trimmed());
                headerLabel->show();
            } else {
                layout->removeWidget(headerLabel);
                headerLabel->hide();
            }
        }
    });
}

GraphWidget::~GraphWidget() {}
