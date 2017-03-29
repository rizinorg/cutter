#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <QDockWidget>

class MainWindow;

namespace Ui {
class Dashboard;
}

class Dashboard : public QDockWidget
{
    Q_OBJECT

public:
    explicit Dashboard(MainWindow *main, QWidget *parent = 0);
    ~Dashboard();

    void updateContents();

private:
    Ui::Dashboard *ui;

    MainWindow      *main;
};

#endif // DASHBOARD_H
