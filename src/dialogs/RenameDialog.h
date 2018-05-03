#ifndef RENAMEDIALOG_H
#define RENAMEDIALOG_H

#include <QDialog>
#include <memory>

namespace Ui {
class RenameDialog;
}

class RenameDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RenameDialog(QWidget *parent = nullptr);
    ~RenameDialog();

    void setName(QString fcnName);
    QString getName() const;

private slots:
    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

private:
    std::unique_ptr<Ui::RenameDialog> ui;
};

#endif // RENAMEDIALOG_H
