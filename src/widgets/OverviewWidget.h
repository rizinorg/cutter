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

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    RefreshDeferrer *refreshDeferrer;

private slots:
    void updateContents();
};

#endif // OverviewWIDGET_H
