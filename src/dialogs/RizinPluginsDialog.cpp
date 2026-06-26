#include "RizinPluginsDialog.h"

#include "Configuration.h"
#include "common/DisassemblyPreview.h"
#include "common/Helpers.h"
#include "core/Cutter.h"
#include "ui_RizinPluginsDialog.h"

#include <QString>
#include <QTreeWidgetItem>

RizinPluginsDialog::RizinPluginsDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::RizinPluginsDialog)
{
    ui->setupUi(this);

    for (const auto &plugin : Core()->getBinPluginDescriptions()) {
        auto *item = new QTreeWidgetItem();
        item->setText(0, plugin.name);
        item->setText(1, plugin.description);
        item->setText(2, plugin.license);
        item->setText(3, plugin.type);
        ui->rzBinTreeWidget->addTopLevelItem(item);
    }
    ui->rzBinTreeWidget->sortByColumn(0, Qt::AscendingOrder);
    qhelpers::adjustColumns(ui->rzBinTreeWidget, 0);

    for (const auto &plugin : Core()->getRIOPluginDescriptions()) {
        auto *item = new QTreeWidgetItem();
        item->setText(0, plugin.name);
        item->setText(1, plugin.description);
        item->setText(2, plugin.license);
        item->setText(3, plugin.permissions);
        ui->rzIOTreeWidget->addTopLevelItem(item);
    }
    ui->rzIOTreeWidget->sortByColumn(0, Qt::AscendingOrder);
    qhelpers::adjustColumns(ui->rzIOTreeWidget, 0);

    for (const auto &plugin : Core()->getRCorePluginDescriptions()) {
        auto *item = new QTreeWidgetItem();
        item->setText(0, plugin.name);
        item->setText(1, plugin.description);
        item->setText(2, plugin.license);
        ui->rzCoreTreeWidget->addTopLevelItem(item);
    }
    ui->rzCoreTreeWidget->sortByColumn(0, Qt::AscendingOrder);
    qhelpers::adjustColumns(ui->rzCoreTreeWidget, 0);

    for (const auto &plugin : Core()->getRAsmPluginDescriptions()) {
        auto *item = new QTreeWidgetItem();
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
        ui->rzAsmTreeWidget->addTopLevelItem(item);
    }
    ui->rzAsmTreeWidget->sortByColumn(0, Qt::AscendingOrder);
    ui->rzAsmTreeWidget->setStyleSheet(DisassemblyPreview::getToolTipStyleSheet());
    qhelpers::adjustColumns(ui->rzAsmTreeWidget, 0);

    const int cpuCol = 2;
    if (ui->rzAsmTreeWidget->columnWidth(cpuCol) > 200) {
        ui->rzAsmTreeWidget->setColumnWidth(cpuCol, 200);
    }
}

RizinPluginsDialog::~RizinPluginsDialog() {}
