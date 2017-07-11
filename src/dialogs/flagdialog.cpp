
#include "ui_flagdialog.h"
#include "flagdialog.h"

FlagDialog::FlagDialog(IaitoRCore *core, RVA offset, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FlagDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    this->core = core;
    this->offset = offset;

    auto size_validator = new QIntValidator(ui->sizeEdit);
    size_validator->setBottom(1);
    ui->sizeEdit->setValidator(size_validator);
}

FlagDialog::~FlagDialog()
{
    delete ui;
}

void FlagDialog::on_buttonBox_accepted()
{
    QString name = ui->nameEdit->text();
    RVA size = ui->sizeEdit->text().toULongLong();
    core->addFlag(offset, name, size);
}

void FlagDialog::on_buttonBox_rejected()
{
    close();
}