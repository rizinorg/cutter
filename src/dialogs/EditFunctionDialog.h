#pragma once

#include <QDialog>
#include <memory>

namespace Ui {
class EditFunctionDialog;
}

class EditFunctionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditFunctionDialog(QWidget *parent = nullptr);
    ~EditFunctionDialog();

private slots:
    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

private:
    std::unique_ptr<Ui::EditFunctionDialog> ui;

    bool eventFilter(QObject *obj, QEvent *event);
};
