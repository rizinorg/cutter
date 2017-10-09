#ifndef CREATENEWDIALOG_H
#define CREATENEWDIALOG_H

#include <QDialog>
#include "MainWindow.h"
#include <memory>

namespace Ui
{
    class CreateNewDialog;
}

class CreateNewDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateNewDialog(QWidget *parent = 0);
    ~CreateNewDialog();

private slots:
    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_exampleButton_clicked();

    void on_buttonCreate_clicked();

private:
    std::unique_ptr<Ui::CreateNewDialog> ui;
    MainWindow *w;
    CutterCore *core;
};

#endif // CREATENEWDIALOG_H
