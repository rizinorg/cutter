#include "WriteCommandsDialogs.h"

#include "Cutter.h"
#include "ui_Base64EnDecodedWriteDialog.h"
#include "ui_DuplicateFromOffsetDialog.h"
#include "ui_IncrementDecrementDialog.h"

#include <QFontDatabase>

Base64EnDecodedWriteDialog::Base64EnDecodedWriteDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::Base64EnDecodedWriteDialog)
{
    ui->setupUi(this);
    ui->decodeRB->click();
}

Base64EnDecodedWriteDialog::~Base64EnDecodedWriteDialog() {}

Base64EnDecodedWriteDialog::Mode Base64EnDecodedWriteDialog::getMode() const
{
    return ui->decodeRB->isChecked() ? Decode : Encode;
}

QByteArray Base64EnDecodedWriteDialog::getData() const
{
    return ui->base64LineEdit->text().toUtf8();
}

IncrementDecrementDialog::IncrementDecrementDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::IncrementDecrementDialog)
{
    ui->setupUi(this);
    ui->incrementRB->click();

    ui->nBytesCB->addItems(QStringList() << tr("Byte") << tr("Word") << tr("Dword") << tr("Qword"));

    ui->valueLE->setValidator(
            new QRegularExpressionValidator(QRegularExpression("[0-9a-fA-Fx]{1,18}"), ui->valueLE));
}

IncrementDecrementDialog::~IncrementDecrementDialog() {}

IncrementDecrementDialog::Mode IncrementDecrementDialog::getMode() const
{
    return ui->incrementRB->isChecked() ? Increase : Decrease;
}

ut8 IncrementDecrementDialog::getNBytes() const
{
    // Shift left to keep index powered by two
    // This is used to create the w1, w2, w4 and w8 commands based on the selected index.
    return static_cast<ut8>(1 << ui->nBytesCB->currentIndex());
}

ut64 IncrementDecrementDialog::getValue() const
{
    return Core()->math(ui->valueLE->text());
}

DuplicateFromOffsetDialog::DuplicateFromOffsetDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::DuplicateFromOffsetDialog)
{
    ui->setupUi(this);
    ui->bytesLabel->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    ui->offsetLE->setValidator(new QRegularExpressionValidator(
            QRegularExpression("[0-9a-fA-Fx]{1,18}"), ui->offsetLE));
    connect(ui->offsetLE, &QLineEdit::textChanged, this, &DuplicateFromOffsetDialog::refresh);
    connect(ui->nBytesSB, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
            &DuplicateFromOffsetDialog::refresh);
}

DuplicateFromOffsetDialog::~DuplicateFromOffsetDialog() {}

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
    const QSignalBlocker sb(Core());
    auto buf = Core()->ioRead(getOffset(), (int)getNBytes());

    // Add space every two characters for word wrap in hex sequence
    const QRegularExpression re { "(.{2})" };
    auto bytes = QString(buf).replace(re, "\\1 ").trimmed();
    ui->bytesLabel->setText(bytes);
}
