#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>
#include <memory>
#include <QtNetwork/QNetworkReply>

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

    /**
     * @fn AboutDialog::on_checkForUpdatesButton_clicked()
     *
     * @brief Initiates process of checking for updates.
    */
    void on_checkForUpdatesButton_clicked();

    /**
     * @fn AboutDialog::on_updatesCheckBox_stateChanged(int state)
     *
     * @brief Changes value of autoUpdateEnabled option in settings.
    */
    void on_updatesCheckBox_stateChanged(int state);

private:
    std::unique_ptr<Ui::AboutDialog> ui;
};

#endif // ABOUTDIALOG_H
