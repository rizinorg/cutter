#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <memory>
#include "CutterDockWidget.h"

class MainWindow;

namespace Ui {
class Dashboard;
}

class Dashboard : public QDockWidget
{
    Q_OBJECT

public:
    explicit Dashboard(MainWindow *main);
    ~Dashboard();

private slots:
    void updateContents();
    void on_certificateButton_clicked();
    void on_versioninfoButton_clicked();

private:
    std::unique_ptr<Ui::Dashboard>   ui;
};

#endif // DASHBOARD_H
