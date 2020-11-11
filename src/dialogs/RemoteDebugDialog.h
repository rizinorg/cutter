#ifndef REMOTEDEBUGDIALOG_H
#define REMOTEDEBUGDIALOG_H

#include <QDialog>
#include <QListWidgetItem>
#include <memory>

namespace Ui {
class RemoteDebugDialog;
}

/**
 * @brief Dialog for connecting to remote debuggers
 */
class RemoteDebugDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RemoteDebugDialog(QWidget *parent = nullptr);
    ~RemoteDebugDialog();

    QString getUri() const;
    bool validate();

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
    void clearAll();
    void removeItem();
    void itemClicked(QListWidgetItem *item);

private:
    int getPort() const;
    int getDebugger() const;
    QString getIpOrPath() const;

    bool validatePath();
    bool validateIp();
    bool validatePort();

    bool fillRecentIpList();
    void fillFormData(QString formdata);

    void checkIfEmpty();

    std::unique_ptr<Ui::RemoteDebugDialog> ui;
};

#endif // REMOTE_DEBUG_DIALOG
