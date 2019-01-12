#ifndef CUTTERWIDGET_H
#define CUTTERWIDGET_H

#include <QDockWidget>

class MainWindow;

class CutterDockWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit CutterDockWidget(MainWindow *main, QAction *action = nullptr);
    ~CutterDockWidget() override;
    bool eventFilter(QObject *object, QEvent *event) override;

public slots:
    void toggleDockWidget(bool show);

private:
    QAction *action;

    /**
     * @brief doRefresh tells if the widget must refresh its content.
     */
    bool doRefresh = false;
    void refreshIfNeeded();

protected:
    void closeEvent(QCloseEvent *event) override;
    bool isVisibleToUser();
    virtual void refreshContent() = 0;
};

#endif // CUTTERWIDGET_H
