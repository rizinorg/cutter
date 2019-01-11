#ifndef MINIGRAPHWIDGET_H
#define MINIGRAPHWIDGET_H

#include "CutterDockWidget.h"

class MainWindow;
class MiniGraphView;

class MiniGraphWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit MiniGraphWidget(MainWindow *main, QAction *action = nullptr);
    ~MiniGraphWidget();
    MiniGraphView *graphView;

private:

};

#endif // MINIGRAPHWIDGET_H
