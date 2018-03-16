#ifndef CUTTERWIDGET_H
#define CUTTERWIDGET_H

#include <QDockWidget>

class MainWindow;

class CutterWidget : public QDockWidget
{

    Q_OBJECT

public:
    explicit CutterWidget(MainWindow *main, QAction *action = nullptr);
    ~CutterWidget();
public slots:
    void toggleDockWidget(bool show);


private:
    QAction *action;

protected:
    void closeEvent(QCloseEvent * event) override;
};

#endif // CUTTERWIDGET_H
