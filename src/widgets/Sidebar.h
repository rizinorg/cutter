#ifndef SIDEBAR_H
#define SIDEBAR_H

#include <QWidget>
#include <memory>

class MainWindow;

namespace Ui {
class SideBar;
}

class SideBar : public QWidget
{
    Q_OBJECT

public:
    explicit SideBar(MainWindow *main);
    ~SideBar();

private slots:

    void on_tabsButton_clicked();

    void on_lockButton_clicked();

    void on_calcInput_textChanged(const QString &arg1);

    void on_asm2hex_clicked();

    void on_hex2asm_clicked();

    void on_respButton_toggled(bool checked);

    void on_refreshButton_clicked();

private:
    std::unique_ptr<Ui::SideBar> ui;
    MainWindow  *main;
};

#endif // SIDEBAR_H
