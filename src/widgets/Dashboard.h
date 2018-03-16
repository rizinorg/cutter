#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <memory>
#include "CutterWidget.h"

class MainWindow;

namespace Ui
{
    class Dashboard;
}

class Dashboard : public CutterWidget
{
    Q_OBJECT

public:
    explicit Dashboard(MainWindow *main, QAction *action = nullptr);
    ~Dashboard();

private slots:
    void updateContents();

private:
    std::unique_ptr<Ui::Dashboard>   ui;
};

#endif // DASHBOARD_H
