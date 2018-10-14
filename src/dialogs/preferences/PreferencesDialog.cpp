#include <QDialogButtonBox>

#include "PreferencesDialog.h"
#include "ui_PreferencesDialog.h"

#include "AppearanceOptionsWidget.h"
#include "AsmOptionsWidget.h"
#include "GraphOptionsWidget.h"
#include "DebugOptionsWidget.h"

#include "PreferenceCategory.h"

#include "utils/Helpers.h"
#include "utils/Configuration.h"


PreferencesDialog::PreferencesDialog(QWidget *parent)
    : QDialog(parent),
      ui(new Ui::PreferencesDialog)
{
    setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);

    QList<PreferenceCategory> prefs {
        {
            "Appearance",
            new AppearanceOptionsWidget(this),
                    QIcon(":/img/icons/cog_light.svg")
        },
        {
            "Assembly",
            new AsmOptionsWidget(this),
                    QIcon(":/img/icons/disas_light.svg"),
            {
                {
                    "Graph",
                    new GraphOptionsWidget(this),
                            QIcon(":/img/icons/graph_light.svg")
                },
            }
        },
        {
            "Debug",
            new DebugOptionsWidget(this),
                    QIcon(":/img/icons/bug_light.svg")
        }
    };

    for (auto &c : prefs)
        c.addItem(*ui->configCategories, *ui->configPanel);

    connect(ui->configCategories, &QTreeWidget::currentItemChanged,
            this, &PreferencesDialog::changePage);

    connect(ui->saveButtons, &QDialogButtonBox::clicked,
            this, &PreferencesDialog::buttonBoxClicked);

    QTreeWidgetItem *defitem = ui->configCategories->topLevelItem(0);
    ui->configCategories->setCurrentItem(defitem, 0);
}

PreferencesDialog::~PreferencesDialog()
{
}

void PreferencesDialog::showSection(PreferencesDialog::Section section)
{
    QTreeWidgetItem *defitem;
    switch (section) {
    case Section::General:
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
    if (previous) {
        QString lastCategory = previous->text(0);
        AbstractOptionWidget *currentWidget =
                qobject_cast<AbstractOptionWidget *>(ui->configPanel->currentWidget());
        if (currentWidget->getIsChanged()) {
            int ret = showNotAppliedDialog();
            if (ret == QMessageBox::Apply)
                apply(lastCategory, currentWidget);
            else if (ret == QMessageBox::Discard)
                discard(lastCategory, currentWidget);
            else if (ret == QMessageBox::Cancel) {
                disconnect(ui->configCategories, &QTreeWidget::currentItemChanged,
                        this, &PreferencesDialog::changePage);
                ui->configCategories->setCurrentItem(previous);
//                ui->configCategories->setCurrentIndex(ui->configCategories->currentIndex());
                connect(ui->configCategories, &QTreeWidget::currentItemChanged,
                        this, &PreferencesDialog::changePage);
                return;
            }
        }
    }

    if (!current)
        current = previous;

    int index = current->data(0, Qt::UserRole).toInt();

    if (index)
        ui->configPanel->setCurrentIndex(index - 1);
}

void PreferencesDialog::buttonBoxClicked(QAbstractButton* b)
{
    AbstractOptionWidget *currWidget = qobject_cast<AbstractOptionWidget *>(ui->configPanel->currentWidget());
    QString currCategory = ui->configCategories->currentItem()->text(0);
    int ret = -1;
    switch (ui->saveButtons->buttonRole(b)) {
    case QDialogButtonBox::AcceptRole:
    case QDialogButtonBox::RejectRole:
        if (currWidget->getIsChanged())
            ret = showNotAppliedDialog();
        if (ret == QMessageBox::Apply)
            apply(currCategory, currWidget);
        else if (ret == QMessageBox::Discard)
            discard(currCategory, currWidget);
        else if (ret == QMessageBox::Cancel)
            break;
        close();
        break;
    case QDialogButtonBox::ApplyRole:
        apply(currCategory, currWidget);
        break;
    default:
        break;
    }
}

int PreferencesDialog::showNotAppliedDialog() const
{
    QMessageBox mb;
    mb.setWindowTitle(tr("Options apply"));
    mb.setText(tr("You have some unsaved changes. Do you want to apply it?"));
    mb.setStandardButtons(QMessageBox::Apply | QMessageBox::Discard | QMessageBox::Cancel);
    return mb.exec();
}

void PreferencesDialog::apply(const QString& category, QWidget *currentWidget)
{
    if (category == "Appearance") {
        AppearanceOptionsWidget *w = qobject_cast<AppearanceOptionsWidget *>(currentWidget);
        w->apply();
    } else if (category == "Assembly") {
        AsmOptionsWidget *w = qobject_cast<AsmOptionsWidget *>(currentWidget);
        w->apply();
    } else if (category == "Graph") {
        GraphOptionsWidget *w = qobject_cast<GraphOptionsWidget *>(currentWidget);
        w->apply();
    } else if (category == "Debug") {
        DebugOptionsWidget *w = qobject_cast<DebugOptionsWidget *>(currentWidget);
        w->apply();
    }
}

void PreferencesDialog::discard(const QString& category, QWidget *currentWidget)
{

    if (category == "Appearance") {
        AppearanceOptionsWidget *w = qobject_cast<AppearanceOptionsWidget *>(currentWidget);
        w->discard();
    } else if (category == "Assembly") {
        AsmOptionsWidget *w = qobject_cast<AsmOptionsWidget *>(currentWidget);
        w->discard();
    } else if (category == "Graph") {
        GraphOptionsWidget *w = qobject_cast<GraphOptionsWidget *>(currentWidget);
        w->discard();
    } else if (category == "Debug") {
        DebugOptionsWidget *w = qobject_cast<DebugOptionsWidget *>(currentWidget);
        w->discard();
    }
}
