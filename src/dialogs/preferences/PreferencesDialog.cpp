#include "PreferencesDialog.h"

#include "AnalysisOptionsWidget.h"
#include "AppearanceOptionsWidget.h"
#include "AsmOptionsWidget.h"
#include "DebugOptionsWidget.h"
#include "GraphOptionsWidget.h"
#include "InitializationFileEditor.h"
#include "InterfaceOptionsWidget.h"
#include "PluginsOptionsWidget.h"
#include "PreferenceCategory.h"
#include "RizinConfigOptionsWidget.h"
#include "ShortcutOptionsWidget.h"
#include "SymbolsOptionsWidget.h"
#include "common/Configuration.h"
#include "common/Helpers.h"
#include "ui_PreferencesDialog.h"

#include <QDialogButtonBox>

PreferencesDialog::PreferencesDialog(MainWindow *parent)
    : QDialog(parent), ui(new Ui::PreferencesDialog), mainWindow(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    QList<PreferenceCategory> prefs {

        { tr("Disassembly"),
          new AsmOptionsWidget(this),
          QIcon(":/img/icons/disas.svg"),
          {
                  { tr("Graph"), new GraphOptionsWidget(this), QIcon(":/img/icons/graph.svg") },
          } },
        { tr("Debug"), new DebugOptionsWidget(this), QIcon(":/img/icons/bug.svg") },
        { tr("Appearance"), new AppearanceOptionsWidget(this), QIcon(":/img/icons/polar.svg") },
        { tr("Plugins"), new PluginsOptionsWidget(this), QIcon(":/img/icons/plugins.svg") },
        { tr("Initialization Script"), new InitializationFileEditor(this),
          QIcon(":/img/icons/initialization.svg") },
        { tr("Analysis"),
          new AnalysisOptionsWidget(this),
          QIcon(":/img/icons/cog_light.svg"),
          {
                  { tr("Symbols"), new SymbolsOptionsWidget(this),
                    QIcon(":/img/icons/symbol_options.svg") },
          } },
        { tr("Shortcuts"), new ShortcutOptionsWidget(this), QIcon(":/img/icons/edit.svg") },
        { tr("Interface"), new InterfaceOptionsWidget(this), QIcon(":/img/icons/layout.svg") },
        { tr("Rizin Config"), new RizinConfigOptionsWidget(this), QIcon(":/img/icons/rizin.svg") }
    };

    for (auto &c : prefs) {
        c.addItem(*ui->configCategories, *ui->configPanel);
    }

    connect(ui->configCategories, &QTreeWidget::currentItemChanged, this,
            &PreferencesDialog::changePage);
    connect(ui->saveButtons, &QDialogButtonBox::accepted, this, &QWidget::close);

    QTreeWidgetItem *defitem = ui->configCategories->topLevelItem(0);
    ui->configCategories->setCurrentItem(defitem, 0);

    connect(Config(), &Configuration::interfaceThemeChanged, this,
            &PreferencesDialog::chooseThemeIcons);
    chooseThemeIcons();
}

PreferencesDialog::~PreferencesDialog() {}

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
    if (!current) {
        current = previous;
    }

    const int index = current->data(0, Qt::UserRole).toInt();

    if (index) {
        ui->configPanel->setCurrentIndex(index - 1);
    }
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
        { QStringLiteral("Initialization Script"), QStringLiteral("initialization.svg") },
        { QStringLiteral("Analysis"), QStringLiteral("cog_light.svg") },
        { QStringLiteral("Symbols"), QStringLiteral("symbol_options.svg") },
        { QStringLiteral("Shortcuts"), QStringLiteral("edit.svg") },
        { QStringLiteral("Interface"), QStringLiteral("layout.svg") },
        { QStringLiteral("Rizin Config"), QStringLiteral("rizin.svg") },
    };
    QList<QPair<void *, QString>> supportedIconsNames;

    foreach (const auto &p, kCategoryIconsNames) {
        QList<QTreeWidgetItem *> items =
                ui->configCategories->findItems(p.first, Qt::MatchContains | Qt::MatchRecursive, 0);
        if (items.isEmpty()) {
            continue;
        }
        supportedIconsNames.append({ items.first(), p.second });
    }

    // Set the correct icon for the QAction
    qhelpers::setThemeIcons(supportedIconsNames, [](void *obj, const QIcon &icon) {
        // here we have our setter functor, in this case it is almost "unique" because it specified
        // the column in `setIcon` call
        static_cast<QTreeWidgetItem *>(obj)->setIcon(0, icon);
    });
}

MainWindow *PreferencesDialog::getMainWindow()
{
    return mainWindow;
}
