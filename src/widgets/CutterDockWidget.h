#ifndef CUTTERWIDGET_H
#define CUTTERWIDGET_H

#include <QDockWidget>

class MainWindow;

class CutterDockWidget : public QDockWidget
{

    Q_OBJECT

public:
    explicit CutterDockWidget(MainWindow *main, QAction *action = nullptr);
    ~CutterDockWidget();
public slots:
    void toggleDockWidget(bool show);


private:
    QAction *action;

protected:
    void closeEvent(QCloseEvent *event) override;
};

#endif // CUTTERWIDGET_H
