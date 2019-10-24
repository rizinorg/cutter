#include "VersionInfoDialog.h"
#include "ui_VersionInfoDialog.h"

#include "common/Helpers.h"

#include <QJsonArray>
#include <QStringList>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTreeWidget>

VersionInfoDialog::VersionInfoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VersionInfoDialog),
    core(Core())
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    // Get version information
    fillVersionInfo();
}

VersionInfoDialog::~VersionInfoDialog() {}

void VersionInfoDialog::fillVersionInfo()
{

    QJsonDocument doc = Core()->getFileVersionInfo();

    // Case ELF
    if (doc.object().contains("verneed")) {
        QJsonObject verneed = doc.object()["verneed"].toArray().first().toObject();
        QJsonObject versym = doc.object()["versym"].toArray().first().toObject();

        // Set labels
        ui->leftLabel->setText("Version symbols");
        ui->rightLabel->setText("Version need");

        //Left tree
        QTreeWidgetItem *secNameItemL = new QTreeWidgetItem();
        secNameItemL->setText(0, "Section name:");
        secNameItemL->setText(1, versym["section_name"].toString());
        ui->leftTreeWidget->addTopLevelItem(secNameItemL);

        QTreeWidgetItem *addrItemL = new QTreeWidgetItem();
        addrItemL->setText(0, "Address:");
        addrItemL->setText(1, RAddressString(versym["address"].toDouble()));
        ui->leftTreeWidget->addTopLevelItem(addrItemL);

        QTreeWidgetItem *offItemL = new QTreeWidgetItem();
        offItemL->setText(0, "Offset:");
        offItemL->setText(1, RAddressString(versym["offset"].toDouble()));
        ui->leftTreeWidget->addTopLevelItem(offItemL);

        QTreeWidgetItem *linkItemL = new QTreeWidgetItem();
        linkItemL->setText(0, "Link:");
        linkItemL->setText(1, QString::number(versym["link"].toDouble()));
        ui->leftTreeWidget->addTopLevelItem(linkItemL);

        QTreeWidgetItem *linkNameItemL = new QTreeWidgetItem();
        linkNameItemL->setText(0, "Link section name:");
        linkNameItemL->setText(1, versym["link_section_name"].toString());
        ui->leftTreeWidget->addTopLevelItem(linkNameItemL);

        QTreeWidgetItem *entriesItemL = new QTreeWidgetItem();
        entriesItemL->setText(0, "Entries:");
        for (QJsonValue val : versym["entries"].toArray()) {
            QJsonObject obj = val.toObject();
            QTreeWidgetItem *tempItem = new QTreeWidgetItem();
            tempItem->setText(0, RAddressString(obj["idx"].toDouble()));
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
        addrItemR->setText(1, RAddressString(verneed["address"].toDouble()));
        ui->rightTreeWidget->addTopLevelItem(addrItemR);

        QTreeWidgetItem *offItemR = new QTreeWidgetItem();
        offItemR->setText(0, "Offset:");
        offItemR->setText(1, RAddressString(verneed["offset"].toDouble()));
        ui->rightTreeWidget->addTopLevelItem(offItemR);

        QTreeWidgetItem *linkItemR = new QTreeWidgetItem();
        linkItemR->setText(0, "Link:");
        linkItemR->setText(1, QString::number(verneed["link"].toDouble()));
        ui->rightTreeWidget->addTopLevelItem(linkItemR);

        QTreeWidgetItem *linkNameItemR = new QTreeWidgetItem();
        linkNameItemR->setText(0, "Link section name:");
        linkNameItemR->setText(1, verneed["link_section_name"].toString());
        ui->rightTreeWidget->addTopLevelItem(linkNameItemR);

        QTreeWidgetItem *entriesItemR = new QTreeWidgetItem();
        entriesItemR->setText(0, "Entries:");
        for (QJsonValue parentVal : verneed["entries"].toArray()) {
            QJsonObject parentObj = parentVal.toObject();
            QTreeWidgetItem *parentItem = new QTreeWidgetItem();
            QString parentString;
            parentItem->setText(0, RAddressString(parentObj["idx"].toDouble()));
            parentString.append("Version: " + QString::number(parentObj["vn_version"].toDouble()) + "\t");
            parentString.append("File: " + parentObj["file_name"].toString());
            parentItem->setText(1, parentString);

            for (QJsonValue childVal : parentObj["vernaux"].toArray()) {
                QJsonObject childObj = childVal.toObject();
                QTreeWidgetItem *childItem = new QTreeWidgetItem();
                QString childString;
                childItem->setText(0, RAddressString(childObj["idx"].toDouble()));
                childString.append("Name: " + childObj["name"].toString() + "\t");
                childString.append("Flags: " + childObj["flags"].toString() + "\t");
                childString.append("Version: " + QString::number(childObj["version"].toDouble()));
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
    else if (doc.object().contains("VS_FIXEDFILEINFO")) {
        QJsonObject vs = doc.object()["VS_FIXEDFILEINFO"].toObject();
        QJsonObject strings = doc.object()["StringTable"].toObject();

        // Set labels
        ui->leftLabel->setText("VS Fixed file info");
        ui->rightLabel->setText("String table");

        // Left tree
        for (QString key : vs.keys()) {
            QTreeWidgetItem *tempItem = new QTreeWidgetItem();
            tempItem->setText(0, key);
            if (vs[key].isDouble())
                tempItem->setText(1, RHexString(vs[key].toDouble()));
            else
                tempItem->setText(1, vs[key].toString());
            ui->leftTreeWidget->addTopLevelItem(tempItem);

            // Adjust columns to content
            qhelpers::adjustColumns(ui->leftTreeWidget, 0);
        }

        // Right tree
        for (QString key : strings.keys()) {
            QTreeWidgetItem *tempItem = new QTreeWidgetItem();
            tempItem->setText(0, key);
            tempItem->setText(1, strings[key].toString());
            ui->rightTreeWidget->addTopLevelItem(tempItem);

            // Adjust columns to content
            qhelpers::adjustColumns(ui->rightTreeWidget, 0);
        }
    }
}
