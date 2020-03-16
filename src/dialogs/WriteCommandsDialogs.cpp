#include "WriteCommandsDialogs.h"
#include "ui_Base64EnDecodedWriteDialog.h"
#include "ui_IncrementDecrementDialog.h"
#include "ui_DuplicateFromOffsetDialog.h"
#include "Cutter.h"

#include <cmath>

Base64EnDecodedWriteDialog::Base64EnDecodedWriteDialog(QWidget* parent) : QDialog(parent),
    ui(new Ui::Base64EnDecodedWriteDialog)
{
    ui->setupUi(this);
    ui->decodeRB->click();
}

Base64EnDecodedWriteDialog::Mode Base64EnDecodedWriteDialog::getMode() const
{
    return ui->decodeRB->isChecked() ? Decode : Encode;
}

QByteArray Base64EnDecodedWriteDialog::getData() const
{
    return ui->base64LineEdit->text().toUtf8();
}

IncrementDecrementDialog::IncrementDecrementDialog(QWidget* parent) : QDialog(parent),
    ui(new Ui::IncrementDecrementDialog)
{
    ui->setupUi(this);
    ui->incrementRB->click();

    ui->nBytesCB->addItems(QStringList()
                           << tr("byte")
                           << tr("word")
                           << tr("dword")
                           << tr("qword"));

    ui->valueLE->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9a-fA-Fx]{1,18}"), ui->valueLE));
}

IncrementDecrementDialog::Mode IncrementDecrementDialog::getMode() const
{
    return ui->incrementRB->isChecked() ? Increase : Decrease;
}

uint8_t IncrementDecrementDialog::getNBytes() const
{
    return static_cast<uint8_t>(std::pow(2, ui->nBytesCB->currentIndex()));
}

uint64_t IncrementDecrementDialog::getValue() const
{
    bool ok;
    uint64_t val = ui->valueLE->text().toULongLong(&ok, 16);
    return ok ? val : 0;
}

DuplicateFromOffsetDialog::DuplicateFromOffsetDialog(QWidget* parent) : QDialog(parent),
    ui(new Ui::DuplicateFromOffsetDialog)
{
    ui->setupUi(this);
    ui->bytesLabel->setFont(QFont("Monospace"));
    ui->offsetLE->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9a-fA-Fx]{1,18}"), ui->offsetLE));
    connect(ui->offsetLE, &QLineEdit::textChanged, this, &DuplicateFromOffsetDialog::refresh);
    connect(ui->nBytesSB, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &DuplicateFromOffsetDialog::refresh);
}

RVA DuplicateFromOffsetDialog::getOffset() const
{
    return Core()->math(ui->offsetLE->text());
}

size_t DuplicateFromOffsetDialog::getNBytes() const
{
    return static_cast<size_t>(ui->nBytesSB->value());
}

void DuplicateFromOffsetDialog::refresh()
{
    RVA offestFrom = getOffset();

    QSignalBlocker sb(Core());

    // Add space every two characters for word wrap in hex sequence
    QRegularExpression re{"(.{2})"};
    QString bytes = Core()->cmd(QString("p8 %1 @ %2")
    .arg(QString::number(getNBytes()))
    .arg(QString::number(offestFrom)))
    .replace(re, "\\1 ");

    ui->bytesLabel->setText(bytes.trimmed());
}
