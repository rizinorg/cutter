#include "IntervalDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QVBoxLayout>

IntervalDialog::IntervalDialog(const QString &name, QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Set Interval for %1").arg(name));

    auto *mainLayout = new QVBoxLayout(this);
    auto *formLayout = new QFormLayout();

    addressEdit = new QLineEdit(this);
    sizeEdit = new QLineEdit(this);

    const QRegularExpression hexOrIntRegex("^(?:0x[0-9a-fA-F]+|[0-9]+)$");
    const QRegularExpressionValidator *validator =
            new QRegularExpressionValidator(hexOrIntRegex, this);

    addressEdit->setValidator(validator);
    sizeEdit->setValidator(validator);

    formLayout->addRow(tr("Address"), addressEdit);
    formLayout->addRow(tr("Size"), sizeEdit);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(buttonBox);
}

void IntervalDialog::setAddress(ut64 addr)
{
    addressEdit->setText(rzAddressString(addr));
}

void IntervalDialog::setSize(ut64 size)
{
    sizeEdit->setText(rzSizeString(size));
}

QString IntervalDialog::getAddress() const
{
    return addressEdit->text();
}

QString IntervalDialog::getSize() const
{
    return sizeEdit->text();
}
