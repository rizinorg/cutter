#ifndef REMOTEDEBUGDIALOG_H
#define REMOTEDEBUGDIALOG_H

#include <QDialog>
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

    void setIp(QString ip);
    void setPort(QString port);
    void setPath(QString path);
    void setDebugger(QString debugger);
    QString getUri() const;
    QString getIp() const;
    int getPort() const;
    QString getPath() const;
    QString getDebugger() const;
    bool validate();

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
    void onIndexChange();

private:
    void activateGdb();
    void activateWinDbgPipe();
    bool validateIp();
    bool validatePort();
    bool validatePath();

    std::unique_ptr<Ui::RemoteDebugDialog> ui;
};

#endif // REMOTE_DEBUG_DIALOG
