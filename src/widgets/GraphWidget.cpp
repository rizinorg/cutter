#include "core/MainWindow.h"
#include "GraphWidget.h"
#include "DisassemblerGraphView.h"
#include "WidgetShortcuts.h"
#include <QVBoxLayout>

GraphWidget::GraphWidget(MainWindow *main) : MemoryDockWidget(MemoryWidgetType::Graph, main)
{
    setObjectName(main ? main->getUniqueObjectName(getWidgetType()) : getWidgetType());

    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto *layoutWidget = new QWidget(this);
    setWidget(layoutWidget);
    auto *layout = new QVBoxLayout(layoutWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layoutWidget->setLayout(layout);

    header = new QLineEdit(this);
    header->setReadOnly(true);
    layout->addWidget(header);

    graphView = new DisassemblerGraphView(layoutWidget, seekable, main, { &syncAction });
    layout->addWidget(graphView);

    // Title needs to get set after graphView is defined
    updateWindowTitle();

    // getting the name of the class is implementation defined, and cannot be
    // used reliably across different compilers.
    // QShortcut *toggle_shortcut = new QShortcut(widgetShortcuts[typeid(this).name()], main);
    QShortcut *toggle_shortcut = new QShortcut(widgetShortcuts["GraphWidget"], main);
    connect(toggle_shortcut, &QShortcut::activated, this, [=]() { toggleDockWidget(true); });

    connect(graphView, &DisassemblerGraphView::nameChanged, this,
            &MemoryDockWidget::updateWindowTitle);

    connect(this, &QDockWidget::visibilityChanged, this, [=](bool visibility) {
        main->toggleOverview(visibility, this);
        if (visibility) {
            graphView->onSeekChanged(Core()->getOffset());
        }
    });

    QAction *switchAction = new QAction(this);
    switchAction->setShortcut(Qt::Key_Space);
    switchAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    addAction(switchAction);
    connect(switchAction, &QAction::triggered, this,
            [this] { mainWindow->showMemoryWidget(MemoryWidgetType::Disassembly); });

    connect(graphView, &DisassemblerGraphView::graphMoved, this,
            [=]() { main->toggleOverview(true, this); });
    connect(seekable, &CutterSeekable::seekableSeekChanged, this, &GraphWidget::prepareHeader);
    connect(Core(), &CutterCore::functionRenamed, this, &GraphWidget::prepareHeader);
    graphView->installEventFilter(this);
}

QWidget *GraphWidget::widgetToFocusOnRaise()
{
    return graphView;
}

void GraphWidget::closeEvent(QCloseEvent *event)
{
    CutterDockWidget::closeEvent(event);
    emit graphClosed();
}

QString GraphWidget::getWindowTitle() const
{
    return graphView->windowTitle;
}

DisassemblerGraphView *GraphWidget::getGraphView() const
{
    return graphView;
}

QString GraphWidget::getWidgetType()
{
    return "Graph";
}

void GraphWidget::prepareHeader()
{
    RzAnalysisFunction *f = Core()->functionIn(seekable->getOffset());
    char *str = f ? rz_analysis_function_get_signature(f) : nullptr;
    if (!str) {
        header->hide();
        return;
    }
    header->show();
    header->setText(str);
    free(str);
}
