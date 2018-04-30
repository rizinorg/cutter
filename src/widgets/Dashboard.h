#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <memory>
#include "CutterDockWidget.h"

class MainWindow;

namespace Ui {
class Dashboard;
}

class Dashboard : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit Dashboard(MainWindow *main, QAction *action = nullptr);
    ~Dashboard();

private slots:
    void updateContents();
    void on_certificateButton_clicked();
    void on_versioninfoButton_clicked();

private:
    std::unique_ptr<Ui::Dashboard>   ui;
};

#endif // DASHBOARD_H
