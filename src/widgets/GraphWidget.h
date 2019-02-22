#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H

#include "CutterDockWidget.h"

class MainWindow;
class DisassemblerGraphView;

class GraphWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit GraphWidget(MainWindow *main, QAction *action = nullptr);
    ~GraphWidget();
    DisassemblerGraphView *graphView;

private:
    void closeEvent(QCloseEvent *event) override;

signals:
    void graphClose();
    void graphEmpty();
};

#endif // GRAPHWIDGET_H
