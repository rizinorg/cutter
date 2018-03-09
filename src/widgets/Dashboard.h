#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <memory>

#include <QDockWidget>
#include <QTreeView>

class MainWindow;

namespace Ui
{
    class Dashboard;
}

class Dashboard : public QDockWidget
{
    Q_OBJECT

public:
    explicit Dashboard(MainWindow *main, QWidget *parent = 0);
    ~Dashboard();

private slots:
    void updateContents();
    void on_certificateButton_clicked();

private:
    std::unique_ptr<Ui::Dashboard>   ui;
    MainWindow      *main;
    QTreeView **ppcertificate_view;
};

#endif // DASHBOARD_H
