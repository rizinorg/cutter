#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>
#include <memory>

namespace Ui {
class AboutDialog;
}

class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent = nullptr);
    ~AboutDialog();

private slots:
    void on_buttonBox_rejected();
    void on_showVersionButton_clicked();
    void on_showPluginsButton_clicked();

private:
    std::unique_ptr<Ui::AboutDialog> ui;
};

#endif // ABOUTDIALOG_H
