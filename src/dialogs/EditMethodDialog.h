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
    void setMethod(const BinClassMethodDescription &meth);

    QString getClass();
    BinClassMethodDescription getMethod();

    static bool showDialog(const QString &title, const QString &className, BinClassMethodDescription *meth, QWidget *parent = nullptr);
    static void newMethod(const QString &className = nullptr, BinClassMethodDescription meth = BinClassMethodDescription());
    static void editMethod(const QString &className, BinClassMethodDescription meth);

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
