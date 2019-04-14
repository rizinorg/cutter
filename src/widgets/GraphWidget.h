#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H

#include "MemoryDockWidget.h"

class MainWindow;
class DisassemblerGraphView;

class GraphWidget : public MemoryDockWidget
{
    Q_OBJECT

public:
    explicit GraphWidget(MainWindow *main, QAction *action = nullptr);
    ~GraphWidget() {}

    DisassemblerGraphView *getGraphView() const;

signals:
    void graphClosed();

protected:
    QWidget *widgetToFocusOnRaise() override;

private:
    void closeEvent(QCloseEvent *event) override;

    DisassemblerGraphView *graphView;
};

#endif // GRAPHWIDGET_H
