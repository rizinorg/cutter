#include "ArenaInfoDialog.h"
#include "ui_ArenaInfoDialog.h"

ArenaInfoDialog::ArenaInfoDialog(QWidget *parent) : QDialog(parent), ui(new Ui::ArenaInfoDialog)
{
    ui->setupUi(this);
}

ArenaInfoDialog::~ArenaInfoDialog()
{
    delete ui;
}
