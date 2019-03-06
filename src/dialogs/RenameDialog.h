#ifndef RENAMEDIALOG_H
#define RENAMEDIALOG_H

#include <QDialog>
#include <memory>

namespace Ui {
class RenameDialog;
}

/**
 * @brief General Dialog for entering a name
 */
class RenameDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RenameDialog(QWidget *parent = nullptr);
    ~RenameDialog();

    void setName(QString fcnName);
    QString getName() const;

    void setPlaceholderText(const QString &text);

    /**
     * @brief Helper function to display and execute the dialog
     *
     * @param title title of the dialog
     * @param name initial name, will be overwritten if the user entered something else
     * @param placeholder placeholder text for the QLineEdit
     * @return whether the dialog was accepted by the user
     */
    static bool showDialog(const QString &title, QString *name, const QString &placeholder, QWidget *parent = nullptr);

private slots:
    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

private:
    std::unique_ptr<Ui::RenameDialog> ui;
};

#endif // RENAMEDIALOG_H
