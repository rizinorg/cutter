#include "R2PluginsDialog.h"
#include "ui_R2PluginsDialog.h"

#include "Cutter.h"
#include "utils/Helpers.h"

R2PluginsDialog::R2PluginsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::R2PluginsDialog)
{
    ui->setupUi(this);

    for(auto plugin : Core()->getRBinPluginDescriptions())
    {
        QTreeWidgetItem* item = new QTreeWidgetItem();
        item->setText(0, plugin.name);
        item->setText(1, plugin.description);
        item->setText(2, plugin.license);
        item->setText(3, plugin.type);
        ui->RBinTreeWidget->addTopLevelItem(item);
    }
    qhelpers::adjustColumns(ui->RBinTreeWidget, 0);

    for(auto plugin : Core()->getRIOPluginDescriptions())
    {
        QTreeWidgetItem* item = new QTreeWidgetItem();
        item->setText(0, plugin.name);
        item->setText(1, plugin.description);
        item->setText(2, plugin.license);
        item->setText(3, plugin.permissions);
        ui->RIOTreeWidget->addTopLevelItem(item);
    }
    qhelpers::adjustColumns(ui->RIOTreeWidget, 0);

    for(auto plugin : Core()->getRCorePluginDescriptions())
    {
        QTreeWidgetItem* item = new QTreeWidgetItem();
        item->setText(0, plugin.name);
        item->setText(1, plugin.description);
        ui->RCoreTreeWidget->addTopLevelItem(item);
    }
    qhelpers::adjustColumns(ui->RCoreTreeWidget, 0);

    for(auto plugin : Core()->getRAsmPlugins())
    {
        QTreeWidgetItem* item = new QTreeWidgetItem();
        item->setText(0, plugin);
        ui->RAsmTreeWidget->addTopLevelItem(item);
    }
}

R2PluginsDialog::~R2PluginsDialog()
{
    delete ui;
}
