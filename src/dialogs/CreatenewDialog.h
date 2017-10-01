#ifndef CREATENEWDIALOG_H
#define CREATENEWDIALOG_H

#include <QDialog>
#include "MainWindow.h"

namespace Ui
{
    class createNewDialog;
}

class createNewDialog : public QDialog
{
    Q_OBJECT

public:
    explicit createNewDialog(QWidget *parent = 0);
    ~createNewDialog();

private slots:
    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_exampleButton_clicked();

    void on_buttonCreate_clicked();

private:
    Ui::createNewDialog *ui;
    MainWindow *w;
};

#endif // CREATENEWDIALOG_H
