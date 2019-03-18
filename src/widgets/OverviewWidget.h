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

    /*
     * @brief if user closed this widget explicitly
     */
    bool userClosed = false;
    bool isVisible = false;
private:
    RefreshDeferrer *refreshDeferrer;
    /**
     * @brief this takes care of scaling the overview when the widget is resized
     */
    void resizeEvent(QResizeEvent *event) override;

private slots:
    /**
     * @brief update the overview
     */
    void updateContents();
    /*
     * @brief override closeEvent to emit graphClose
     */
    void closeEvent(QCloseEvent *event) override;

signals:
    /**
     * @brief emit signal to update the rect
     */
    void resized();
    /*
     * @brief emit signal to notify when this widget is closed
     */
    void graphClose();
};

#endif // OverviewWIDGET_H
