#include "MainWindow.h"
#include "GraphWidget.h"
#include "DisassemblerGraphView.h"
#include "WidgetShortcuts.h"

GraphWidget::GraphWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action)
{
    this->setObjectName("Graph");
    this->setAllowedAreas(Qt::AllDockWidgetAreas);
    this->graphView = new DisassemblerGraphView(this);
    this->setWidget(graphView);

    // getting the name of the class is implementation defined, and cannot be
    // used reliably across different compilers.
    //QShortcut *toggle_shortcut = new QShortcut(widgetShortcuts[typeid(this).name()], main);
    QShortcut *toggle_shortcut = new QShortcut(widgetShortcuts["GraphWidget"], main);
    connect(toggle_shortcut, &QShortcut::activated, this, [ = ]() {
            toggleDockWidget(true); 
            main->updateDockActionChecked(action);
    });

    connect(this, &QDockWidget::visibilityChanged, this, [ = ](bool visibility) {
        main->toggleOverview(visibility, this);
        if (visibility) {
            Core()->setMemoryWidgetPriority(CutterCore::MemoryWidgetType::Graph);
            this->graphView->refreshView();
        }
    });

    connect(graphView, &DisassemblerGraphView::graphMoved, this, [ = ]() {
        main->toggleOverview(true, this);
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
