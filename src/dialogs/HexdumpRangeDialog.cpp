#include "HexdumpRangeDialog.h"
#include "ui_HexdumpRangeDialog.h"

#include <QRegExpValidator>
#include <QPushButton>
#include <cstdint>
#include "core/Cutter.h"

HexdumpRangeDialog::HexdumpRangeDialog(QWidget *parent, bool allowEmpty) :
    QDialog(parent),
    ui(new Ui::HexdumpRangeDialog),
    allowEmpty(allowEmpty)
{
    ui->setupUi(this);
    QRegExpValidator *v = new QRegExpValidator(QRegExp("(?:0[xX])?[0-9a-fA-F]+"), this);
    ui->lengthLineEdit->setValidator(v);
    ui->startAddressLineEdit->setValidator(v);
    ui->endAddressLineEdit->setValidator(v);

    //subscribe to a text change slot
    connect(ui->startAddressLineEdit, &QLineEdit::textEdited, this, &HexdumpRangeDialog::textEdited);
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

bool HexdumpRangeDialog::empty()
{
    return emptyRange;
}

unsigned long long HexdumpRangeDialog::getStartAddress() const
{
    return startAddress;
}

unsigned long long HexdumpRangeDialog::getEndAddress() const
{
    return endAddress;
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

void HexdumpRangeDialog::open(ut64 start)
{
    setStartAddress(start);
    setModal(false);
    show();
    activateWindow();
    raise();
}

bool HexdumpRangeDialog::validate()
{
    bool warningVisibile = false;
    startAddress = Core()->math(ui->startAddressLineEdit->text());
    endAddress = 0;
    ut64 length = 0;
    emptyRange = true;
    if (ui->endAddressRadioButton->isChecked()) {
        endAddress = Core()->math(ui->endAddressLineEdit->text());
        if (endAddress > startAddress) {
            length = endAddress - startAddress;
            ui->lengthLineEdit->setText(
                QString("0x%1").arg(length, 0, 16));
            this->endAddress = endAddress - 1;
            emptyRange = false;
        } else  if (endAddress == startAddress) {
            ui->lengthLineEdit->setText("0");
            return allowEmpty;
        } else {
            ui->lengthLineEdit->setText("Invalid");
            return false;
        }
    } else {
        //we edited the length, so update the end address to be start address + length
        length = Core()->math(ui->lengthLineEdit->text());
        if (length == 0) {
            ui->endAddressLineEdit->setText("Empty");
            return allowEmpty;
        } else if (UINT64_MAX - startAddress < length - 1) {
            ui->endAddressLineEdit->setText("Invalid");
            return false;
        } else {
            endAddress = startAddress + length - 1;
            emptyRange = false;
            if (endAddress == UINT64_MAX) {
                ui->endAddressLineEdit->setText(
                    QString("2^64"));
            } else {
                ui->endAddressLineEdit->setText(
                    QString("0x%1").arg(endAddress + 1, 0, 16));
            }
        }
    }

    // Warn the user for potentially heavy operation
    if (length > 0x25000) {
        warningVisibile = true;
    }

    ui->selectionWarningLabel->setVisible(warningVisibile);
    return true;
}

void HexdumpRangeDialog::textEdited()
{
    bool valid = validate();
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
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
