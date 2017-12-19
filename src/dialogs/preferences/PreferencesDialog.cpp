
#include <QDialogButtonBox>

#include "PreferencesDialog.h"
#include "ui_PreferencesDialog.h"

#include "GeneralOptionsWidget.h"
#include "AsmOptionsWidget.h"
#include "GraphOptionsWidget.h"

#include "utils/Helpers.h"
#include "utils/Configuration.h"


PreferencesDialog::PreferencesDialog(QWidget *parent)
        : QDialog(parent),
          ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);

#define ADD_TAB(c) { auto w = new c(this); ui->tabWidget->addTab(w, w->windowTitle()); }
    ADD_TAB(GeneralOptionsWidget)
    ADD_TAB(AsmOptionsWidget)
    ADD_TAB(GraphOptionsWidget)
#undef ADD_TAB
}

PreferencesDialog::~PreferencesDialog()
{
}

void PreferencesDialog::showSection(PreferencesDialog::Section section)
{
    switch(section)
    {
        case Section::General:
            ui->tabWidget->setCurrentIndex(0);
            break;
        case Section::Disassembly:
            ui->tabWidget->setCurrentIndex(1);
            break;
    }
}
