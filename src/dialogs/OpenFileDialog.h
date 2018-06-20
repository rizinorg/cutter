#ifndef OPENFILEDIALOG_H
#define OPENFILEDIALOG_H

#include <QDialog>
#include <memory>
#include "Cutter.h"

namespace Ui {
class OpenFileDialog;
}

class OpenFileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OpenFileDialog(QWidget *parent = nullptr);
    ~OpenFileDialog();

private slots:
    void on_selectFileButton_clicked();
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

private:
    std::unique_ptr<Ui::OpenFileDialog> ui;
};

#endif // OPENFILEDIALOG_H
