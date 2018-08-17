#ifndef EDITMETHODDIALOG_H
#define EDITMETHODDIALOG_H

#include <QDialog>
#include <memory>

#include "Cutter.h"

namespace Ui {
class EditMethodDialog;
}

class EditMethodDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditMethodDialog(QWidget *parent = nullptr);
    ~EditMethodDialog();

    void setClass(const QString &className);
    void setMethod(const ClassMethodDescription &meth);

    QString getClass();
    ClassMethodDescription getMethod();

    static bool showDialog(const QString &title, const QString &className, ClassMethodDescription *meth, QWidget *parent = nullptr);
    static void newMethod(const QString &className = nullptr, ClassMethodDescription meth = ClassMethodDescription());
    static void editMethod(const QString &className, ClassMethodDescription meth);

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

    void updateVirtualUI();
    void validateInput();

private:
    std::unique_ptr<Ui::EditMethodDialog> ui;

    bool inputValid();
};

#endif // EDITMETHODDIALOG_H
