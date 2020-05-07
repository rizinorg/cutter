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
    enum Type {
        Flag,
        Function,
        String,
        Class
    };

    explicit RenameDialog(QWidget *parent = nullptr, Type type = Type::Flag);
    ~RenameDialog();

    // Type that represents the object to be renamed
    Type type;

    /**
     * @brief Set the value of the flag name Line Edit to \a name.
     * 
     * @param name The flag name to be filled in the Line Edit.
     */
    void setName(QString name);
    QString getName() const;

    /**
     * @brief Set the value of the realname Line Edit to \a name.
     * 
     * @param name The realname to be filled in the Line Edit.
     */
    void setRealName(QString name);

    /**
     * @brief Get the value of the realname Line Edit
     * 
     * @return QString
     */
    QString getRealName() const;

    void setDialogType(Type type);
    void setPlaceholderText(const QString &text);

    /**
     * @brief Helper function to display and execute the dialog
     *
     * @param title title of the dialog
     * @param name initial name, will be overwritten if the user entered something else
     * @param placeholder placeholder text for the QLineEdit
     * @return whether the dialog was accepted by the user
     */
    static bool showDialog(const QString &title, QString *name, const QString &placeholder,
                           Type type = Type::Flag, QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event);
    void accept();

private slots:
    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

    void autoRenameStateChanged(int state);
    void realnameTextEdited(QString text);
private:
    std::unique_ptr<Ui::RenameDialog> ui;
    QString flagPrefix = QString();
    QString flagSuffix = QString();
};

#endif // RENAMEDIALOG_H
