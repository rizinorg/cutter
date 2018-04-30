#include "VersionInfoDialog.h"
#include "ui_VersionInfoDialog.h"

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

void VersionInfoDialog::fillVersionInfo(){

    QJsonDocument doc = Core()->getFileVersionInfo();

    QJsonObject verneed = doc.object()["verneed"].toArray().first().toObject();
    QJsonObject versym = doc.object()["versym"].toArray().first().toObject();

    // Version symbols section
    this->ui->versSymEdit->setText(versym["section_name"].toString());
    this->ui->addrSymEdit->setText(RAddressString(versym["address"].toDouble()));
    this->ui->offSymEdit->setText(RAddressString(versym["offset"].toDouble()));
    this->ui->linkSymEdit->setText(QString::number(versym["link"].toDouble()));
    this->ui->linknameSymEdit->setText(versym["link_section_name"].toString());

    foreach (QJsonValue val, versym["entries"].toArray()) {
        QJsonObject obj = val.toObject();
        QTreeWidgetItem *tempItem = new QTreeWidgetItem();
        tempItem->setText(0, RAddressString(obj["idx"].toDouble()));
        tempItem->setText(1, obj["value"].toString());
        ui->versSymTree->addTopLevelItem(tempItem);
    }

    // Version need section
    this->ui->versNeedEdit->setText(verneed["section_name"].toString());
    this->ui->addrNeedEdit->setText(RAddressString(verneed["address"].toDouble()));
    this->ui->offNeedEdit->setText(RAddressString(verneed["offset"].toDouble()));
    this->ui->linkNeedEdit->setText(QString::number(verneed["link"].toDouble()));
    this->ui->linknameNeedEdit->setText(verneed["link_section_name"].toString());

    foreach (QJsonValue parentVal, verneed["entries"].toArray()) {
        QJsonObject parentObj = parentVal.toObject();
        QTreeWidgetItem *parentItem = new QTreeWidgetItem();
        QString parentString;
        parentItem->setText(0, RAddressString(parentObj["idx"].toDouble()));
        parentString.append("Version: " + QString::number(parentObj["vn_version"].toDouble()) + "\t");
        parentString.append("File: " + parentObj["file_name"].toString());
        parentItem->setText(1, parentString);
        foreach (QJsonValue childVal, parentObj["vernaux"].toArray()) {
            QJsonObject childObj = childVal.toObject();
            QTreeWidgetItem *childItem = new QTreeWidgetItem();
            QString childString;
            childItem->setText(0, RAddressString(childObj["idx"].toDouble()));
            childString.append("Name: " + childVal["name"].toString() + "\t");
            childString.append("Flags: " + childVal["flags"].toString() + "\t");
            childString.append("Version: " + QString::number(childVal["version"].toDouble()));
            childItem->setText(1, childString);
            parentItem->addChild(childItem);
        }
        ui->versNeedTree->addTopLevelItem(parentItem);
    }
}
