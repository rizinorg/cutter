#ifndef SAVEPROJECTDIALOG_H
#define SAVEPROJECTDIALOG_H

#include <QDialog>
#include <memory>

namespace Ui
{
    class SaveProjectDialog;
}

class SaveProjectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SaveProjectDialog(QWidget *parent = 0);
    ~SaveProjectDialog();

private slots:
	void on_buttonBox_accepted();
    void on_buttonBox_rejected();

private:
    std::unique_ptr<Ui::SaveProjectDialog> ui;
};

#endif // ABOUTDIALOG_H
