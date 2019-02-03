#ifndef LOADNEWTYPESDIALOG_H
#define LOADNEWTYPESDIALOG_H

#include <QDialog>
#include <memory>

namespace Ui {
class LoadNewTypesDialog;
}

class LoadNewTypesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoadNewTypesDialog(QWidget *parent = nullptr);
    ~LoadNewTypesDialog();

private slots:
    void on_selectFileButton_clicked();

    void on_buttonBox_accepted();

private:
    std::unique_ptr<Ui::LoadNewTypesDialog> ui;
};

#endif // LOADNEWTYPESDIALOG_H
