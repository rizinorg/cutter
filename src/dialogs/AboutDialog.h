#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>
#include <QtNetwork/QNetworkReply>

#include <memory>

namespace Ui {
class AboutDialog;
}

/**
 * @brief Dialog that displays memory allocation details and offsets for a specific Arena.
 */
class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent = nullptr);
    ~AboutDialog();

private slots:
    void onButtonBoxRejected();
    void onShowVersionButtonClicked();
    void onShowPluginsButtonClicked();
    void onIssueClicked();

    /**
     * @brief Initiates process of checking for updates.
     */
    void onCheckForUpdatesButtonClicked();

    /**
     * @brief Changes value of autoUpdateEnabled option in settings.
     */
    void onUpdatesCheckBoxStateChanged(int state);

private:
    std::unique_ptr<Ui::AboutDialog> ui;

    QString buildQtVersionString(void);
};

#endif // ABOUTDIALOG_H
