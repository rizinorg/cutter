#ifndef BASEFIND_DIALOG_H
#define BASEFIND_DIALOG_H

#include <QDialog>
#include <QListWidgetItem>

#include <core/Cutter.h>
#include <memory>

namespace Ui {
class BaseFindDialog;
}

/**
 * @brief Dialog for configuring and launching the BaseFind analysis
 */
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
    void onButtonBoxAccepted();
    void onButtonBoxRejected();

private:
    std::unique_ptr<Ui::BaseFindDialog> ui;
};

#endif // BASEFIND_DIALOG_H
