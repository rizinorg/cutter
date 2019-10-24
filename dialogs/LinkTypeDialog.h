#ifndef LINKTYPEDIALOG_H
#define LINKTYPEDIALOG_H

#include "core/Cutter.h"
#include <QDialog>

namespace Ui {
class LinkTypeDialog;
}

class LinkTypeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LinkTypeDialog(QWidget *parent = nullptr);
    ~LinkTypeDialog();

    /**
     * @brief Sets the default type which will be displayed in the combo box
     * @param type Default type to be used as default type
     */
    void setDefaultType(const QString &type);

    /**
     * @brief Sets the value of the default address which will be displayed
     * If the given address is linked to a type, then it also sets the default
     * type to the currently linked type
     * @param address The address to be used as default address
     * @return true iff the given address string was valid
     */
    bool setDefaultAddress(const QString &address);

private slots:

    /**
     * @brief Overrides the done() method of QDialog
     * On clicking the Ok button, it links a valid address to a type.
     * If "(No Type)" is selected as type, it removes the link.
     * In case of an invalid address, it displays error message
     * @param r The value which will be returned by exec()
     */
    void done(int r) override;

    /**
     * @brief Executed whenever the text inside exprLineEdit changes
     * If expression evaluates to valid address, it is displayed in addressLineEdit
     * Otherwise "Invalid Address" is shown in addressLineEdit
     * @param text The current value of exprLineEdit
     */
    void on_exprLineEdit_textChanged(const QString &text);

private:
    Ui::LinkTypeDialog *ui;

    bool addrValid;

    /**
     * @brief Used for finding the type which is linked to the given address
     * @param address
     * @return The type linked to "address" if it exists, or empty string otherwise
     */
    QString findLinkedType(RVA address);
};

#endif // LINKTYPEDIALOG_H
