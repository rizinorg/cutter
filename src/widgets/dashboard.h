#ifndef DASHBOARD_H
#define DASHBOARD_H

#include "dockwidget.h"

class MainWindow;

namespace Ui
{
    class Dashboard;
}

class Dashboard : public DockWidget
{
    Q_OBJECT

public:
    explicit Dashboard(MainWindow *main, QWidget *parent = 0);
    ~Dashboard();

    void setup() override;

    void refresh() override;

private:
    void updateContents();

    Ui::Dashboard   *ui;
    MainWindow      *main;
};

#endif // DASHBOARD_H
