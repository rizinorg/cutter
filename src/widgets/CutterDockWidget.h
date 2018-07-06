#ifndef CUTTERWIDGET_H
#define CUTTERWIDGET_H

#include <QDockWidget>

class MainWindow;

class CutterDockWidget : public QObject
{
    Q_OBJECT

public:
    CutterDockWidget(MainWindow *main, QDockWidget *dockWidget, QAction *action);
    ~CutterDockWidget() override;

    QDockWidget *getDockWidget()    { return dockWidget; }
    QAction *getAction()            { return action; }

    void show()                     { dockWidget->show(); }
    void raise()                    { dockWidget->raise(); }

public slots:
    void toggleDockWidget(bool show);

private:
    QDockWidget *dockWidget;
    QAction *action;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};

#endif // CUTTERWIDGET_H
