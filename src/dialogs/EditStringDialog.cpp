#include "EditStringDialog.h"

#include "ui_EditStringDialog.h"

EditStringDialog::EditStringDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::EditStringDialog {})
{
    ui->setupUi(this);
    ui->spinBoxSize->setMinimum(0);
    ui->lineEditAddress->setMinimumWidth(150);
    ui->spinBoxSize->setFocus();
    ui->comboBoxType->addItems({ "Auto", "ASCII/Latin1", "UTF-8" });
    connect(ui->checkBoxAutoSize, &QCheckBox::toggled, ui->spinBoxSize, &QSpinBox::setDisabled);
}

EditStringDialog::~EditStringDialog() {}

void EditStringDialog::setStringStartAddress(uint64_t address)
{
    ui->lineEditAddress->setText(QString::number(address, 16));
}

bool EditStringDialog::getStringStartAddress(uint64_t &returnValue) const
{
    bool status = false;
    returnValue = ui->lineEditAddress->text().toLongLong(&status, 16);
    return status;
}

void EditStringDialog::setStringSizeValue(uint32_t size)
{
    ui->spinBoxSize->setValue(size);
}

int EditStringDialog::getStringSizeValue() const
{
    if (ui->checkBoxAutoSize->isChecked()) {
        return -1;
    }

    return ui->spinBoxSize->value();
}

EditStringDialog::StringType EditStringDialog::getStringType() const
{
    const int indexVal = ui->comboBoxType->currentIndex();

    switch (indexVal) {
    case 0: {
        return EditStringDialog::StringType::Auto;
    }
    case 1: {
        return EditStringDialog::StringType::ASCII_LATIN1;
    }
    case 2: {
        return EditStringDialog::StringType::UTF8;
    }
    default:
        return EditStringDialog::StringType::Auto;
    }
}
