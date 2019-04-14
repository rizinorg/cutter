#ifndef OVERVIEWWIDGET_H
#define OVERVIEWWIDGET_H

#include "CutterDockWidget.h"

class MainWindow;
class OverviewView;
class GraphWidget;

class OverviewWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit OverviewWidget(MainWindow *main, QAction *action = nullptr);
    ~OverviewWidget();

private:
    OverviewView *graphView;
    bool isAvailable = false;
    bool userOpened = false;

    GraphWidget *targetGraphWidget;

    RefreshDeferrer *graphDataRefreshDeferrer;

    /**
     * @brief this takes care of scaling the overview when the widget is resized
     */
    void resizeEvent(QResizeEvent *event) override;

    void setIsAvailable(bool isAvailable);
    void setUserOpened(bool userOpened);

private slots:
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

    /**
     * @brief update the view in the target widget when the range rect in the overview is moved
     */
    void updateTargetView();

    /**
     * @brief update the content of the graph (blocks, edges) in the contained graphView from the target widget
     */
    void updateGraphData();

    /**
     * @brief update the rect to show the current view in the target widget
     */
    void updateRangeRect();

    void targetClosed();

signals:
    /**
     * @brief emit signal to update the rect
     */
    void resized();

    /**
     * @sa getIsAvailable()
     */
    void isAvailableChanged(bool isAvailable);

    /**
     * @sa getUserOpened()
     */
    void userOpenedChanged(bool userOpened);

public:
    GraphWidget *getTargetGraphWidget() { return targetGraphWidget; }
    void setTargetGraphWidget(GraphWidget *widget);

    /**
     * @brief whether this widget makes sense to be show, i.e. the menu entry should be enabled
     */
    bool getIsAvailable() const         { return isAvailable; }

    /**
     * @brief whether this widget is desired to be shown in general
     *
     * Will be false when the user closed the overview explicitly.
     * Also corresponds to the checked state of the menu entry for this widget.
     */
    bool getUserOpened() const          { return userOpened; }

    OverviewView *getGraphView() const  { return graphView; }
};

#endif // OverviewWIDGET_H
