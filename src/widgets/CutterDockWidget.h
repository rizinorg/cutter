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

signals:
    void becameVisibleToUser();

private:
    QAction *action;

    bool isVisibleToUserCurrent;
    void updateIsVisibleToUser();

protected:
    void closeEvent(QCloseEvent *event) override;
    bool isVisibleToUser()      { return isVisibleToUserCurrent; }
};

#endif // CUTTERWIDGET_H
