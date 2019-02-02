#ifndef EDITMETHODDIALOG_H
#define EDITMETHODDIALOG_H

#include <QDialog>
#include <QComboBox>

#include <memory>

#include "Cutter.h"

namespace Ui {
class EditMethodDialog;
}

class EditMethodDialog : public QDialog
{
    Q_OBJECT

public:
    /*!
     * \param classFixed whether the user should be able to change the class. If false, a QComboBox will be shown, otherwise a plain QLabel.
     */
    explicit EditMethodDialog(bool classFixed, QWidget *parent = nullptr);
    ~EditMethodDialog();

    void setClass(const QString &className);
    void setMethod(const AnalMethodDescription &desc);

    QString getClass();
    AnalMethodDescription getMethod();

    /*!
     * \brief Helper function to display the dialog
     *
     * \param title title of the dialog
     * \param classFixed whether the user should be able to change the class
     * \param className initial class name, will be overwritten if the user changed the class
     * \param desc initial data for the method information
     * \return whether the dialog was accepted by the user
     */
    static bool showDialog(const QString &title, bool classFixed, QString *className, AnalMethodDescription *desc, QWidget *parent = nullptr);

    /*!
     * \brief Show the dialog to add a new method a given class
     */
    static void newMethod(QString className = nullptr, const QString &meth = QString(), QWidget *parent = nullptr);

    /*!
     * \brief Show the dialog to edit a given method of a given class
     */
    static void editMethod(const QString &className, const QString &meth, QWidget *parent = nullptr);

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

    void updateVirtualUI();
    void validateInput();

private:
    std::unique_ptr<Ui::EditMethodDialog> ui;

    QComboBox *classComboBox = nullptr;
    QLabel *classLabel = nullptr;
    /*!
     * This will only be used when the dialog was created with classFixed = true in order to remember the class name.
     */
    QString fixedClass;

    bool inputValid();
};

#endif // EDITMETHODDIALOG_H
