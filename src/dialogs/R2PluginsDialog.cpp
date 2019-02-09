#include "R2PluginsDialog.h"
#include "ui_R2PluginsDialog.h"

#include "Cutter.h"
#include "common/Helpers.h"
#include "plugins/PluginManager.h"

R2PluginsDialog::R2PluginsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::R2PluginsDialog)
{
    ui->setupUi(this);

    for (const auto &plugin : Core()->getRBinPluginDescriptions()) {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, plugin.name);
        item->setText(1, plugin.description);
        item->setText(2, plugin.license);
        item->setText(3, plugin.type);
        ui->RBinTreeWidget->addTopLevelItem(item);
    }
    qhelpers::adjustColumns(ui->RBinTreeWidget, 0);

    for (const auto &plugin : Core()->getRIOPluginDescriptions()) {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, plugin.name);
        item->setText(1, plugin.description);
        item->setText(2, plugin.license);
        item->setText(3, plugin.permissions);
        ui->RIOTreeWidget->addTopLevelItem(item);
    }
    qhelpers::adjustColumns(ui->RIOTreeWidget, 0);

    for (const auto &plugin : Core()->getRCorePluginDescriptions()) {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, plugin.name);
        item->setText(1, plugin.description);
        ui->RCoreTreeWidget->addTopLevelItem(item);
    }
    qhelpers::adjustColumns(ui->RCoreTreeWidget, 0);

    for (const auto &plugin : Core()->getRAsmPluginDescriptions()) {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, plugin.name);
        item->setText(1, plugin.architecture);
        item->setText(2, plugin.cpus);
        item->setText(3, plugin.version);
        item->setText(4, plugin.description);
        item->setText(5, plugin.license);
        item->setText(6, plugin.author);
        ui->RAsmTreeWidget->addTopLevelItem(item);
    }
    qhelpers::adjustColumns(ui->RAsmTreeWidget, 0);

    for (CutterPlugin *plugin : Plugins()->getPlugins()) {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, plugin->getName());
        item->setText(1, plugin->getDescription());
        item->setText(2, plugin->getVersion());
        item->setText(3, plugin->getAuthor());
        ui->CutterTreeWidget->addTopLevelItem(item);
    }
}

R2PluginsDialog::~R2PluginsDialog()
{
    delete ui;
}
