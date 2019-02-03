#include "dialogs/LoadNewTypes.h"
#include "ui_LoadNewTypes.h"

LoadNewTypes::LoadNewTypes(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoadNewTypes)
{
    ui->setupUi(this);
}

LoadNewTypes::~LoadNewTypes()
{
    delete ui;
}
