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
    /*
     * \brief this takes care of scaling the overview when the widget is resized
     */
    void resizeEvent(QResizeEvent *event) override;

private slots:
    /*
     * \brief update the overview
     */
    void updateContents();

signals:
    /*
     * \brief emit signal to update the rect
     */
    void resized();
};

#endif // OverviewWIDGET_H
