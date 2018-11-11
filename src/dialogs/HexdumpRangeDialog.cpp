#include "HexdumpRangeDialog.h"
#include "ui_HexdumpRangeDialog.h"

HexdumpRangeDialog::HexdumpRangeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HexdumpRangeDialog)
{
    ui->setupUi(this);

    //subscribe to a text change slot
    connect(ui->endAddressLineEdit, &QLineEdit::textEdited, this, &HexdumpRangeDialog::textEdited);
    connect(ui->lengthLineEdit, &QLineEdit::textEdited, this, &HexdumpRangeDialog::textEdited);
    connect(ui->endAddressRadioButton, &QRadioButton::clicked, this,
            &HexdumpRangeDialog::on_radioButtonClicked);
    connect(ui->lengthRadioButton, &QRadioButton::clicked, this,
            &HexdumpRangeDialog::on_radioButtonClicked);

}

HexdumpRangeDialog::~HexdumpRangeDialog()
{
    delete ui;
}

QString HexdumpRangeDialog::getStartAddress() const
{
    return ui->startAddressLineEdit->text();
}

QString HexdumpRangeDialog::getEndAddress() const
{
    return ui->endAddressLineEdit->text();
}

QString HexdumpRangeDialog::getLength() const
{
    return ui->lengthLineEdit->text();
}

bool HexdumpRangeDialog::getEndAddressRadioButtonChecked() const
{
    return ui->endAddressRadioButton->isChecked();
}

bool HexdumpRangeDialog::getLengthRadioButtonChecked() const
{
    return ui->lengthRadioButton->isChecked();
}

void HexdumpRangeDialog::setStartAddress(ut64 start)
{
    ui->startAddressLineEdit->setText(
        QString("0x%1").arg(start, 0, 16));
}

void HexdumpRangeDialog::textEdited()
{
    QString startAddress = ui->startAddressLineEdit->text();
    ut64 endAddress;
    ut64 length;

    if (sender() == ui->endAddressLineEdit) {
        length = Core()->math(getEndAddress() + " - " + startAddress);

        ui->lengthLineEdit->setText(
            QString("0x%1").arg(length, 0, 16));
    } else if ( sender() == ui->lengthLineEdit) {
        //we edited the length, so update the end address to be start address + length
        endAddress = Core()->math(startAddress + " + " + getLength());
        ui->endAddressLineEdit->setText(
            QString("0x%1").arg(endAddress, 0, 16));
    }

}

void HexdumpRangeDialog::on_radioButtonClicked(bool checked)
{

    if (sender() == ui->endAddressRadioButton && checked == true) {
        ui->lengthLineEdit->setEnabled(false);
        ui->endAddressLineEdit->setEnabled(true);
        ui->endAddressLineEdit->setFocus();
    } else if (sender() == ui->lengthRadioButton && checked == true) {
        ui->lengthLineEdit->setEnabled(true);
        ui->endAddressLineEdit->setEnabled(false);
        ui->lengthLineEdit->setFocus();
    }

}
