#include "FlagDialog.h"
#include "ui_FlagDialog.h"

#include <QIntValidator>
#include "core/Cutter.h"


FlagDialog::FlagDialog(RVA offset, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FlagDialog),
    offset(offset)
{
    // Setup UI
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
    flag = r_flag_get_i(Core()->core()->flags, offset);

    auto size_validator = new QIntValidator(ui->sizeEdit);
    size_validator->setBottom(1);
    ui->sizeEdit->setValidator(size_validator);
    if (flag) {
        ui->nameEdit->setText(flag->name);
        ui->labelAction->setText(tr("Edit flag at %1").arg(RAddressString(offset)));
    } else {
        ui->labelAction->setText(tr("Add flag at %1").arg(RAddressString(offset)));
    }

    // Connect slots
    connect(ui->buttonBox, &QDialogButtonBox::accepted,
        this, &FlagDialog::buttonBoxAccepted);
    connect(ui->buttonBox, &QDialogButtonBox::rejected,
        this, &FlagDialog::buttonBoxRejected);
}

FlagDialog::~FlagDialog() {}

void FlagDialog::buttonBoxAccepted()
{
    RVA size = ui->sizeEdit->text().toULongLong();
    QString name = ui->nameEdit->text();

    if (name.isEmpty()) {
        if (flag) {
            // Empty name and flag exists -> delete the flag
            Core()->delFlag(flag->offset);
        } else {
            // Flag was not existing and we gave an empty name, do nothing
        }
    } else {
        if (flag) {
            // Name provided and flag exists -> rename the flag
            Core()->renameFlag(flag->name, name);
            flag->size = size;
        } else {
            // Name provided and flag does not exist -> create the flag
            Core()->addFlag(offset, name, size);
        }
    }
    close();
}

void FlagDialog::buttonBoxRejected()
{
    close();
}
