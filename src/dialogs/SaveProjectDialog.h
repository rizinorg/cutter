#ifndef SAVEPROJECTDIALOG_H
#define SAVEPROJECTDIALOG_H

#include <QDialog>
#include <QAbstractButton>

#include <memory>

namespace Ui {
class SaveProjectDialog;
}

class SaveProjectDialog : public QDialog
{
    Q_OBJECT

public:
    enum Result { Saved, Rejected, Destructive };

    explicit SaveProjectDialog(bool quit, QWidget *parent = nullptr);
    ~SaveProjectDialog();

    virtual void accept() override;
    virtual void reject() override;

private slots:
    void on_buttonBox_clicked(QAbstractButton *button);
    void on_selectProjectsDirButton_clicked();

private:
    std::unique_ptr<Ui::SaveProjectDialog> ui;
};

#endif // SAVEPROJECTDIALOG_H
