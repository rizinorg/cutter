#include "FlagDialog.h"
#include "ui_FlagDialog.h"

#include <QIntValidator>
#include "core/Cutter.h"

FlagDialog::FlagDialog(RVA offset, QWidget *parent, QString flagNameHint)
    : QDialog(parent),
      ui(new Ui::FlagDialog),
      offset(offset),
      flagName(flagNameHint),
      flagOffset(RVA_INVALID)
{
    // Setup UI
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
    if (!flagName.isEmpty()) {
        flagOffset = offset;
    } else {
        RzCoreLocked core(Core());
        RzFlagItem *flag = rz_flag_get_i(core->flags, offset);
        if (flag) {
            flagName = QString(flag->name);
            flagOffset = flag->offset;
        }
    }

    auto size_validator = new QIntValidator(ui->sizeEdit);
    size_validator->setBottom(1);
    ui->sizeEdit->setValidator(size_validator);
    if (!flagName.isEmpty()) {
        ui->nameEdit->setText(flagName);
        ui->labelAction->setText(tr("Edit flag at %1").arg(RzAddressString(offset)));
    } else {
        ui->labelAction->setText(tr("Add flag at %1").arg(RzAddressString(offset)));
    }

    // Connect slots
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &FlagDialog::buttonBoxAccepted);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &FlagDialog::buttonBoxRejected);
}

FlagDialog::~FlagDialog() {}

void FlagDialog::buttonBoxAccepted()
{
    RVA size = ui->sizeEdit->text().toULongLong();
    QString name = ui->nameEdit->text();

    if (name.isEmpty()) {
        if (flagOffset != RVA_INVALID) {
            // Empty name and flag exists -> delete the flag
            Core()->delFlag(flagName);
        } else {
            // Flag was not existing and we gave an empty name, do nothing
        }
    } else {
        if (flagOffset != RVA_INVALID) {
            // Name provided and flag exists -> rename the flag
            Core()->renameFlag(flagName, name);
        } else {
            // Name provided and flag does not exist -> create the flag
            Core()->addFlag(offset, name, size);
        }
    }
    close();
    this->setResult(QDialog::Accepted);
}

void FlagDialog::buttonBoxRejected()
{
    close();
    this->setResult(QDialog::Rejected);
}
