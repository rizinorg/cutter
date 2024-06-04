#ifndef BASEFIND_DIALOG_H
#define BASEFIND_DIALOG_H

#include <QDialog>
#include <QListWidgetItem>
#include <memory>

#include <core/Cutter.h>

namespace Ui {
class BaseFindDialog;
}

class BaseFindDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BaseFindDialog(QWidget *parent = nullptr);
    ~BaseFindDialog();

    RzThreadNCores getNCores() const;
    ut32 getPointerSize() const;
    RVA getStartAddress() const;
    RVA getEndAddress() const;
    RVA getAlignment() const;
    ut32 getMinStrLen() const;
    ut32 getMinScore() const;

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

private:
    std::unique_ptr<Ui::BaseFindDialog> ui;
};

#endif // BASEFIND_DIALOG_H
