#ifndef OVERVIEWWIDGET_H
#define OVERVIEWWIDGET_H

#include "CutterDockWidget.h"

class MainWindow;
class OverviewView;

class OverviewWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit OverviewWidget(MainWindow *main, QAction *action = nullptr);
    ~OverviewWidget();
    OverviewView *graphView;

private:
    RefreshDeferrer *refreshDeferrer;

private slots:
    /**
     * @brief update the overview
     */
    void updateContents();
};

#endif // OverviewWIDGET_H
