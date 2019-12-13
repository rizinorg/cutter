#include "EditStringDialog.h"
#include "ui_EditStringDialog.h"

EditStringDialog::EditStringDialog(QWidget *parent)
    :   QDialog(parent)
    ,   ui(new Ui::EditStringDialog)
{
    ui->setupUi(this);
    ui->spinBox_size->setMinimum(0);
    ui->lineEdit_address->setMinimumWidth(150);
    ui->pushButton_ok->setFocus();
    ui->comboBox_type->addItems({"Auto", "ASCII/Latin1", "UTF-8"});
    connect(ui->checkBox_autoSize, &QCheckBox::toggled, ui->spinBox_size, &QSpinBox::setDisabled);
}

EditStringDialog::~EditStringDialog()
{
    delete ui;
}

void EditStringDialog::setStringStartAddress(uint64_t address)
{
    ui->lineEdit_address->setText(QString::number(address, 16));
}

bool EditStringDialog::getStringStartAddress(uint64_t& returnValue) const
{
    bool status = false;
    returnValue = ui->lineEdit_address->text().toLongLong(&status, 16);
    return status;
}

void EditStringDialog::setStringSizeValue(uint32_t size)
{
    ui->spinBox_size->setValue(size);
}

int EditStringDialog::getStringSizeValue() const
{
    if( ui->checkBox_autoSize->isChecked() ) {
        return -1;
    }

    return ui->spinBox_size->value();
}

EditStringDialog::StringType EditStringDialog::getStringType() const
{
    const int indexVal =  ui->comboBox_type->currentIndex();

    switch(indexVal)
    {
    case 0:
    {
        return EditStringDialog::StringType::Auto;
    }
    case 1:
    {
        return EditStringDialog::StringType::ASCII_LATIN1;
    }
    case 2:
    {
        return EditStringDialog::StringType::UTF8;
    }
    default:
        return EditStringDialog::StringType::Auto;
    }
}
