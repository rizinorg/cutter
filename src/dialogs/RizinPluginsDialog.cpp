#include "RizinPluginsDialog.h"
#include "ui_RizinPluginsDialog.h"

#include "Configuration.h"
#include "common/DisassemblyPreview.h"
#include "core/Cutter.h"
#include "common/Helpers.h"
#include "plugins/PluginManager.h"

#include <QString>
#include <QTreeWidgetItem>

RizinPluginsDialog::RizinPluginsDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::RizinPluginsDialog)
{
    ui->setupUi(this);

    for (const auto &plugin : Core()->getBinPluginDescriptions()) {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, plugin.name);
        item->setText(1, plugin.description);
        item->setText(2, plugin.license);
        item->setText(3, plugin.type);
        ui->RzBinTreeWidget->addTopLevelItem(item);
    }
    ui->RzBinTreeWidget->sortByColumn(0, Qt::AscendingOrder);
    qhelpers::adjustColumns(ui->RzBinTreeWidget, 0);

    for (const auto &plugin : Core()->getRIOPluginDescriptions()) {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, plugin.name);
        item->setText(1, plugin.description);
        item->setText(2, plugin.license);
        item->setText(3, plugin.permissions);
        ui->RzIOTreeWidget->addTopLevelItem(item);
    }
    ui->RzIOTreeWidget->sortByColumn(0, Qt::AscendingOrder);
    qhelpers::adjustColumns(ui->RzIOTreeWidget, 0);

    for (const auto &plugin : Core()->getRCorePluginDescriptions()) {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, plugin.name);
        item->setText(1, plugin.description);
        item->setText(2, plugin.license);
        ui->RzCoreTreeWidget->addTopLevelItem(item);
    }
    ui->RzCoreTreeWidget->sortByColumn(0, Qt::AscendingOrder);
    qhelpers::adjustColumns(ui->RzCoreTreeWidget, 0);

    for (const auto &plugin : Core()->getRAsmPluginDescriptions()) {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, plugin.name);
        item->setText(1, plugin.architecture);
        item->setText(2, plugin.cpus);
        if (!plugin.cpus.isEmpty()) {
            QString cpus = plugin.cpus;
            cpus.replace(",", ", ");

            const QFont &fnt = Config()->getFont();
            const QString tooltip =
                    QString("<html><div style='font-family:'%1';font-size:%2pt;'>%3</div></html>")
                            .arg(fnt.family().toHtmlEscaped())
                            .arg(qMax(8, fnt.pointSize() - 1))
                            .arg(cpus.toHtmlEscaped());

            item->setToolTip(2, tooltip);
        }
        item->setText(3, plugin.version);
        item->setText(4, plugin.description);
        item->setText(5, plugin.license);
        item->setText(6, plugin.author);
        item->setText(7, plugin.capabilities);
        item->setText(8, plugin.bits);
        ui->RzAsmTreeWidget->addTopLevelItem(item);
    }
    ui->RzAsmTreeWidget->sortByColumn(0, Qt::AscendingOrder);
    ui->RzAsmTreeWidget->setStyleSheet(DisassemblyPreview::getToolTipStyleSheet());
    qhelpers::adjustColumns(ui->RzAsmTreeWidget, 0);

    int cpuCol = 2;
    if (ui->RzAsmTreeWidget->columnWidth(cpuCol) > 200) {
        ui->RzAsmTreeWidget->setColumnWidth(cpuCol, 200);
    }
}

RizinPluginsDialog::~RizinPluginsDialog()
{
    delete ui;
}
