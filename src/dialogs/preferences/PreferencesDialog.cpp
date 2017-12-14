
#include <QDialogButtonBox>

#include "PreferencesDialog.h"
#include "ui_PreferencesDialog.h"

#include "AsmOptionsWidget.h"

#include "utils/Helpers.h"
#include "utils/Configuration.h"


PreferencesDialog::PreferencesDialog(QWidget *parent)
        : QDialog(parent),
          ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);

    ui->buttonBox->addButton(tr("Save as Defaults"), QDialogButtonBox::ButtonRole::ApplyRole);

    auto asmOptionsWidget = new AsmOptionsWidget(this);
    ui->tabWidget->addTab(asmOptionsWidget, tr("Disassembly"));
}

PreferencesDialog::~PreferencesDialog()
{
}

void PreferencesDialog::showSection(PreferencesDialog::Section section)
{
    switch(section)
    {
        case Section::Disassembly:
            ui->tabWidget->setCurrentIndex(0);
            break;
    }
}

void PreferencesDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    switch (ui->buttonBox->buttonRole(button))
    {
        case QDialogButtonBox::ButtonRole::ApplyRole:
            emit saveAsDefault();
            break;
        case QDialogButtonBox::ButtonRole::ResetRole:
            emit resetToDefault();
            break;
        default:
            break;
    }
}
