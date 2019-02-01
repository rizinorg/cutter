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
    void setMethod(const AnalMethodDescription &desc);

    QString getClass();
    AnalMethodDescription getMethod();

    static bool showDialog(const QString &title, QString *className, AnalMethodDescription *desc, QWidget *parent = nullptr);
    static void newMethod(QString className = nullptr, const QString &meth = QString(), QWidget *parent = nullptr);
    static void editMethod(const QString &className, const QString &meth, QWidget *parent = nullptr);

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
