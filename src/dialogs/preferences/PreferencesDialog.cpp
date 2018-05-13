#include <QDialogButtonBox>

#include "PreferencesDialog.h"
#include "ui_PreferencesDialog.h"

#include "GeneralOptionsWidget.h"
#include "AsmOptionsWidget.h"
#include "GraphOptionsWidget.h"

#include "PreferenceCategory.h"

#include "utils/Helpers.h"
#include "utils/Configuration.h"


PreferencesDialog::PreferencesDialog(QWidget *parent)
    : QDialog(parent),
      ui(new Ui::PreferencesDialog)
{
    setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);

    QList<PreferenceCategory> prefs
    {
        {
            "General",
            new GeneralOptionsWidget(this),
            QIcon(":/img/icons/cog.svg")
        },
        {
            "Assembly",
            new AsmOptionsWidget(this),
            QIcon(":/img/icons/disas.svg"),
            {
                {
                    "Graph",
                    new GraphOptionsWidget(this),
                    QIcon(":/img/icons/graph.svg")
                },
            }
        },
    };

    for (auto &c : prefs)
        c.addItem(*ui->configCategories, *ui->configPanel);

    connect(ui->configCategories,
            SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
            this, SLOT(changePage(QTreeWidgetItem*, QTreeWidgetItem*)));
    connect(ui->saveButtons,
            SIGNAL(accepted()),
            this, SLOT(close()));

    QTreeWidgetItem *defitem = ui->configCategories->topLevelItem(0);
    ui->configCategories->setCurrentItem(defitem, 0);
}

PreferencesDialog::~PreferencesDialog()
{
}

void PreferencesDialog::showSection(PreferencesDialog::Section section)
{
    switch (section) {
    case Section::General:
        ui->configPanel->setCurrentIndex(0);
        break;
    case Section::Disassembly:
        ui->configPanel->setCurrentIndex(1);
        break;
    }
}

void PreferencesDialog::changePage(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    if (!current)
        current = previous;

    int index = current->data(0, Qt::UserRole).toInt();

    if (index)
        ui->configPanel->setCurrentIndex(index-1);
}
