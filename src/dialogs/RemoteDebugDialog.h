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

    /**
     * @brief Generate a URI for given UI input
     */
    QString getUri() const;
    bool validate();

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

    /**
     * @brief Clears the list of recent connections.
     * Triggers when you right click and click on "Clear All" in remote debug dialog.
     */
    void clearAll();

    /**
     * @brief Clears the selected item in the list of recent connections.
     * Triggers when you right click and click on "Remove Item" in remote debug dialog.
     */
    void removeItem();

    /**
     * @brief Fills the form with the selected item's data.
     */
    void itemClicked(QListWidgetItem *item);

private:
    std::unique_ptr<Ui::RemoteDebugDialog> ui;
    int getPort() const;
    int getDebugger() const;
    QString getIpOrPath() const;

    bool validatePath();
    bool validateIp();
    bool validatePort();

    /**
     * @brief Fills recent remote connections.
     */
    bool fillRecentIpList();

    /**
     * @brief Fills the remote debug dialog form from a given URI
     * Eg: gdb://127.0.0.1:8754 or windbg:///tmp/abcd
     */
    void fillFormData(QString formdata);

    /**
     * @brief Checks if the recent connection list is empty or and hide/unhide the table view
     */
    void checkIfEmpty();
};

#endif // REMOTE_DEBUG_DIALOG
