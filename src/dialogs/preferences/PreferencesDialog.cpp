#include "PreferencesDialog.h"
#include "ui_PreferencesDialog.h"

#include "AppearanceOptionsWidget.h"
#include "AsmOptionsWidget.h"
#include "GraphOptionsWidget.h"
#include "DebugOptionsWidget.h"
#include "PluginsOptionsWidget.h"

#include "PreferenceCategory.h"

#include "common/Helpers.h"
#include "common/Configuration.h"

#include <QDialogButtonBox>


PreferencesDialog::PreferencesDialog(QWidget *parent)
    : QDialog(parent),
      ui(new Ui::PreferencesDialog)
{
    setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    QList<PreferenceCategory> prefs {

        {
            tr("Disassembly"),
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
        {
            tr("Debug"),
            new DebugOptionsWidget(this),
            QIcon(":/img/icons/bug.svg")
        },
        {
            tr("Appearance"),
            new AppearanceOptionsWidget(this),
            QIcon(":/img/icons/polar.svg")
        },
        {
            tr("Plugins"),
            new PluginsOptionsWidget(this),
            QIcon(":/img/icons/plugins.svg")
        }
    };

    for (auto &c : prefs)
    {
        c.addItem(*ui->configCategories, *ui->configPanel);
    }

    connect(ui->configCategories,
            SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
            this, SLOT(changePage(QTreeWidgetItem *, QTreeWidgetItem *)));
    connect(ui->saveButtons,
            SIGNAL(accepted()),
            this, SLOT(close()));

    QTreeWidgetItem *defitem = ui->configCategories->topLevelItem(0);
    ui->configCategories->setCurrentItem(defitem, 0);

    connect(Config(), &Configuration::interfaceThemeChanged, this, &PreferencesDialog::chooseThemeIcons);
    chooseThemeIcons();
}

PreferencesDialog::~PreferencesDialog()
{
}

void PreferencesDialog::showSection(PreferencesDialog::Section section)
{
    QTreeWidgetItem *defitem;
    switch (section) {
    case Section::Appearance:
        ui->configPanel->setCurrentIndex(0);
        defitem = ui->configCategories->topLevelItem(0);
        ui->configCategories->setCurrentItem(defitem, 0);
        break;
    case Section::Disassembly:
        ui->configPanel->setCurrentIndex(1);
        defitem = ui->configCategories->topLevelItem(1);
        ui->configCategories->setCurrentItem(defitem, 1);
        break;
    }
}

void PreferencesDialog::changePage(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    if (!current)
        current = previous;

    int index = current->data(0, Qt::UserRole).toInt();

    if (index)
        ui->configPanel->setCurrentIndex(index - 1);
}

void PreferencesDialog::chooseThemeIcons()
{
    // List of QActions which have alternative icons in different themes
    const QList<QPair<QString, QString>> kCategoryIconsNames {
        { QStringLiteral("Debug"), QStringLiteral("bug.svg") },
        { QStringLiteral("Assembly"), QStringLiteral("disas.svg") },
        { QStringLiteral("Graph"), QStringLiteral("graph.svg") },
        { QStringLiteral("Appearance"), QStringLiteral("polar.svg") },
        { QStringLiteral("Plugins"), QStringLiteral("plugins.svg") },
    };
    QList<QPair<void*, QString>> supportedIconsNames;

    foreach (const auto &p, kCategoryIconsNames) {
        QList<QTreeWidgetItem *> items = ui->configCategories->findItems(p.first, Qt::MatchContains|Qt::MatchRecursive, 0);
        if (items.isEmpty()) {
            continue;
        }
        supportedIconsNames.append( { items.first(), p.second } );
    }

    // Set the correct icon for the QAction
    qhelpers::setThemeIcons(supportedIconsNames, [](void *obj, const QIcon &icon) {
        // here we have our setter functor, in this case it is almost "unique" because it specified the column in `setIcon` call
        static_cast<QTreeWidgetItem*>(obj)->setIcon(0, icon);
    });
}
