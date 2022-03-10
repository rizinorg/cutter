#include "VersionInfoDialog.h"
#include "ui_VersionInfoDialog.h"

#include "common/Helpers.h"

#include <QJsonArray>
#include <QStringList>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTreeWidget>

VersionInfoDialog::VersionInfoDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::VersionInfoDialog), core(Core())
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    // Get version information
    fillVersionInfo();
}

VersionInfoDialog::~VersionInfoDialog() {}

void VersionInfoDialog::fillVersionInfo()
{

    CutterJson doc = Core()->getFileVersionInfo();

    // Case ELF
    if (doc["verneed"].valid()) {
        CutterJson verneed = doc["verneed"].first();
        CutterJson versym = doc["versym"].first();

        // Set labels
        ui->leftLabel->setText("Version symbols");
        ui->rightLabel->setText("Version need");

        // Left tree
        QTreeWidgetItem *secNameItemL = new QTreeWidgetItem();
        secNameItemL->setText(0, "Section name:");
        secNameItemL->setText(1, versym["section_name"].toString());
        ui->leftTreeWidget->addTopLevelItem(secNameItemL);

        QTreeWidgetItem *addrItemL = new QTreeWidgetItem();
        addrItemL->setText(0, "Address:");
        addrItemL->setText(1, RzAddressString(versym["address"].toRVA()));
        ui->leftTreeWidget->addTopLevelItem(addrItemL);

        QTreeWidgetItem *offItemL = new QTreeWidgetItem();
        offItemL->setText(0, "Offset:");
        offItemL->setText(1, RzAddressString(versym["offset"].toRVA()));
        ui->leftTreeWidget->addTopLevelItem(offItemL);

        QTreeWidgetItem *linkItemL = new QTreeWidgetItem();
        linkItemL->setText(0, "Link:");
        linkItemL->setText(1, QString::number(versym["link"].toRVA()));
        ui->leftTreeWidget->addTopLevelItem(linkItemL);

        QTreeWidgetItem *linkNameItemL = new QTreeWidgetItem();
        linkNameItemL->setText(0, "Link section name:");
        linkNameItemL->setText(1, versym["link_section_name"].toString());
        ui->leftTreeWidget->addTopLevelItem(linkNameItemL);

        QTreeWidgetItem *entriesItemL = new QTreeWidgetItem();
        entriesItemL->setText(0, "Entries:");
        for (CutterJson obj : versym["entries"]) {
            QTreeWidgetItem *tempItem = new QTreeWidgetItem();
            tempItem->setText(0, RzAddressString(obj["idx"].toRVA()));
            tempItem->setText(1, obj["value"].toString());
            entriesItemL->addChild(tempItem);
        }
        ui->leftTreeWidget->addTopLevelItem(entriesItemL);

        // Adjust columns to content
        qhelpers::adjustColumns(ui->leftTreeWidget, 0);

        // Right tree
        QTreeWidgetItem *secNameItemR = new QTreeWidgetItem();
        secNameItemR->setText(0, "Section name:");
        secNameItemR->setText(1, verneed["section_name"].toString());
        ui->rightTreeWidget->addTopLevelItem(secNameItemR);

        QTreeWidgetItem *addrItemR = new QTreeWidgetItem();
        addrItemR->setText(0, "Address:");
        addrItemR->setText(1, RzAddressString(verneed["address"].toRVA()));
        ui->rightTreeWidget->addTopLevelItem(addrItemR);

        QTreeWidgetItem *offItemR = new QTreeWidgetItem();
        offItemR->setText(0, "Offset:");
        offItemR->setText(1, RzAddressString(verneed["offset"].toRVA()));
        ui->rightTreeWidget->addTopLevelItem(offItemR);

        QTreeWidgetItem *linkItemR = new QTreeWidgetItem();
        linkItemR->setText(0, "Link:");
        linkItemR->setText(1, QString::number(verneed["link"].toSt64()));
        ui->rightTreeWidget->addTopLevelItem(linkItemR);

        QTreeWidgetItem *linkNameItemR = new QTreeWidgetItem();
        linkNameItemR->setText(0, "Link section name:");
        linkNameItemR->setText(1, verneed["link_section_name"].toString());
        ui->rightTreeWidget->addTopLevelItem(linkNameItemR);

        QTreeWidgetItem *entriesItemR = new QTreeWidgetItem();
        entriesItemR->setText(0, "Entries:");
        for (CutterJson parentObj : verneed["entries"]) {
            QTreeWidgetItem *parentItem = new QTreeWidgetItem();
            QString parentString;
            parentItem->setText(0, RzAddressString(parentObj["idx"].toRVA()));
            parentString.append("Version: " + QString::number(parentObj["vn_version"].toSt64())
                                + "\t");
            parentString.append("File: " + parentObj["file_name"].toString());
            parentItem->setText(1, parentString);

            for (CutterJson childObj : parentObj["vernaux"]) {
                QTreeWidgetItem *childItem = new QTreeWidgetItem();
                QString childString;
                childItem->setText(0, RzAddressString(childObj["idx"].toRVA()));
                childString.append("Name: " + childObj["name"].toString() + "\t");
                childString.append("Flags: " + childObj["flags"].toString() + "\t");
                childString.append("Version: " + QString::number(childObj["version"].toSt64()));
                childItem->setText(1, childString);
                parentItem->addChild(childItem);
            }
            entriesItemR->addChild(parentItem);
        }
        ui->rightTreeWidget->addTopLevelItem(entriesItemR);

        // Adjust columns to content
        qhelpers::adjustColumns(ui->rightTreeWidget, 0);

    }

    // Case PE
    else if (doc["VS_FIXEDFILEINFO"].valid()) {
        CutterJson vs = doc["VS_FIXEDFILEINFO"];
        CutterJson strings = doc["StringTable"];

        // Set labels
        ui->leftLabel->setText("VS Fixed file info");
        ui->rightLabel->setText("String table");

        // Left tree
        for (CutterJson property : vs) {
            QTreeWidgetItem *tempItem = new QTreeWidgetItem();
            tempItem->setText(0, property.key());
            if (property.type() == RZ_JSON_INTEGER)
                tempItem->setText(1, RzHexString(property.toRVA()));
            else
                tempItem->setText(1, property.toString());
            ui->leftTreeWidget->addTopLevelItem(tempItem);

            // Adjust columns to content
            qhelpers::adjustColumns(ui->leftTreeWidget, 0);
        }

        // Right tree
        for (CutterJson property : strings) {
            QTreeWidgetItem *tempItem = new QTreeWidgetItem();
            tempItem->setText(0, property.key());
            tempItem->setText(1, property.toString());
            ui->rightTreeWidget->addTopLevelItem(tempItem);

            // Adjust columns to content
            qhelpers::adjustColumns(ui->rightTreeWidget, 0);
        }
    }
}
