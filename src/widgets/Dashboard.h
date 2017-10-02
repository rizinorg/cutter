#ifndef DASHBOARD_H
#define DASHBOARD_H

#include "DockWidget.h"
#include <memory>

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

    std::unique_ptr<Ui::Dashboard>   ui;
    MainWindow      *main;
};

#endif // DASHBOARD_H
