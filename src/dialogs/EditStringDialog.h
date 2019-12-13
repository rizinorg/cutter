#ifndef EDITSTRINGDIALOG_H
#define EDITSTRINGDIALOG_H

#include <QDialog>

namespace Ui {
class EditStringDialog;
}

class EditStringDialog : public QDialog
{
    Q_OBJECT

public:
    enum class StringType {Auto, ASCII_LATIN1, UTF8};
    explicit EditStringDialog(QWidget *parent = nullptr);
    ~EditStringDialog();

    void setStringStartAddress(uint64_t address);
    bool getStringStartAddress(uint64_t &returnValue) const;

    void setStringSizeValue(uint32_t size);
    int getStringSizeValue() const;

    StringType getStringType() const;

private:
    Ui::EditStringDialog *ui;
};

#endif // EDITSTRINGDIALOG_H
