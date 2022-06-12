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
    RzCoreLocked core(Core());
    const RzBinInfo *info = rz_bin_get_info(core->bin);
    // Case ELF
    if (strncmp("elf", info->rclass, 3) == 0) {
        Sdb *sdb = sdb_ns_path(core->sdb, "bin/cur/info/versioninfo/versym", 0);

        // Set labels
        ui->leftLabel->setText("Version symbols");
        ui->rightLabel->setText("Version need");

        // Left tree
        QTreeWidgetItem *secNameItemL = new QTreeWidgetItem();
        secNameItemL->setText(0, "Section name:");
        secNameItemL->setText(1, "");
        ui->leftTreeWidget->addTopLevelItem(secNameItemL);

        QTreeWidgetItem *addrItemL = new QTreeWidgetItem();
        addrItemL->setText(0, "Address:");
        addrItemL->setText(1, RzAddressString(sdb_num_get(sdb, "addr", 0)));
        ui->leftTreeWidget->addTopLevelItem(addrItemL);

        QTreeWidgetItem *offItemL = new QTreeWidgetItem();
        offItemL->setText(0, "Offset:");
        offItemL->setText(1, RzAddressString(sdb_num_get(sdb, "offset", 0)));
        ui->leftTreeWidget->addTopLevelItem(offItemL);

        QTreeWidgetItem *linkItemL = new QTreeWidgetItem();
        linkItemL->setText(0, "Link:");
        linkItemL->setText(1, "");
        ui->leftTreeWidget->addTopLevelItem(linkItemL);

        QTreeWidgetItem *linkNameItemL = new QTreeWidgetItem();
        linkNameItemL->setText(0, "Link section name:");
        linkNameItemL->setText(1, "");
        ui->leftTreeWidget->addTopLevelItem(linkNameItemL);

        QTreeWidgetItem *entriesItemL = new QTreeWidgetItem();
        entriesItemL->setText(0, "Entries:");
        const ut64 num_entries = sdb_num_get(sdb, "num_entries", 0);
        for (size_t i = 0; i < num_entries; ++i) {
            const char *const key = sdb_fmt("entry%zu", i);
            const char *const value = sdb_const_get(sdb, key, 0);
            if (!value) {
                continue;
            }
            auto item = new QTreeWidgetItem();
            item->setText(0, RzAddressString(i));
            item->setText(1, value);
            entriesItemL->addChild(item);
        }
        ui->leftTreeWidget->addTopLevelItem(entriesItemL);

        // Adjust columns to content
        qhelpers::adjustColumns(ui->leftTreeWidget, 0);
        sdb = sdb_ns_path(core->sdb, "bin/cur/info/versioninfo/verneed", 0);
        // Right tree
        QTreeWidgetItem *secNameItemR = new QTreeWidgetItem();
        secNameItemR->setText(0, "Section name:");
        secNameItemR->setText(1, "");
        ui->rightTreeWidget->addTopLevelItem(secNameItemR);

        QTreeWidgetItem *addrItemR = new QTreeWidgetItem();
        addrItemR->setText(0, "Address:");
        addrItemR->setText(1, RzAddressString(sdb_num_get(sdb, "addr", 0)));
        ui->rightTreeWidget->addTopLevelItem(addrItemR);

        QTreeWidgetItem *offItemR = new QTreeWidgetItem();
        offItemR->setText(0, "Offset:");
        offItemR->setText(1, RzAddressString(sdb_num_get(sdb, "offset", 0)));
        ui->rightTreeWidget->addTopLevelItem(offItemR);

        QTreeWidgetItem *linkItemR = new QTreeWidgetItem();
        linkItemR->setText(0, "Link:");
        linkItemR->setText(1, "");
        ui->rightTreeWidget->addTopLevelItem(linkItemR);

        QTreeWidgetItem *linkNameItemR = new QTreeWidgetItem();
        linkNameItemR->setText(0, "Link section name:");
        linkNameItemR->setText(1, "");
        ui->rightTreeWidget->addTopLevelItem(linkNameItemR);

        QTreeWidgetItem *entriesItemR = new QTreeWidgetItem();
        entriesItemR->setText(0, "Entries:");
        for (size_t num_version = 0;; num_version++) {
            char *path_version =
                    sdb_fmt("bin/cur/info/versioninfo/verneed/version%zu", num_version);
            sdb = sdb_ns_path(core->sdb, path_version, 0);
            if (!sdb) {
                break;
            }
            const char *filename = sdb_const_get(sdb, "file_name", 0);
            auto *parentItem = new QTreeWidgetItem();
            QString parentString;
            parentItem->setText(0, RzAddressString(sdb_num_get(sdb, "idx", 0)));
            parentString.append("Version: " + QString::number(sdb_num_get(sdb, "vn_version", 0))
                                + "\t");
            parentString.append("File: " + QString(filename));
            parentItem->setText(1, parentString);

            int num_vernaux = 0;
            while (true) {
                const char *const path_vernaux =
                        sdb_fmt("%s/vernaux%d", path_version, num_vernaux++);
                sdb = sdb_ns_path(core->sdb, path_vernaux, 0);
                if (!sdb) {
                    break;
                }

                auto *childItem = new QTreeWidgetItem();
                childItem->setText(0, RzAddressString(sdb_num_get(sdb, "idx", 0)));
                QString childString =
                        QString("Name: %0\t"
                                "Flags: %1\t"
                                "Version: %2\t")
                                .arg(sdb_const_get(sdb, "name", 0), sdb_const_get(sdb, "flags", 0),
                                     QString::number(sdb_num_get(sdb, "version", 0)));
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
    else if (strncmp("pe", info->rclass, 2) == 0) {
        // Set labels
        ui->leftLabel->setText("VS Fixed file info");
        ui->rightLabel->setText("String table");
        Sdb *sdb = NULL;

        // Left tree
        char *path_version = rz_str_newf("bin/cur/info/vs_version_info/VS_VERSIONINFO%d", 0);
        char *path_fixedfileinfo = rz_str_newf("%s/fixed_file_info", path_version);
        sdb = sdb_ns_path(core->sdb, path_fixedfileinfo, 0);
        free(path_fixedfileinfo);
        if (!sdb) {
            return;
        }
        ut32 file_version_ms = sdb_num_get(sdb, "FileVersionMS", 0);
        ut32 file_version_ls = sdb_num_get(sdb, "FileVersionLS", 0);
        char *file_version =
                rz_str_newf("%u.%u.%u.%u", file_version_ms >> 16, file_version_ms & 0xFFFF,
                            file_version_ls >> 16, file_version_ls & 0xFFFF);
        ut32 product_version_ms = sdb_num_get(sdb, "ProductVersionMS", 0);
        ut32 product_version_ls = sdb_num_get(sdb, "ProductVersionLS", 0);
        char *product_version =
                rz_str_newf("%u.%u.%u.%u", product_version_ms >> 16, product_version_ms & 0xFFFF,
                            product_version_ls >> 16, product_version_ls & 0xFFFF);

        auto item = new QTreeWidgetItem();
        item->setText(0, "Signature");
        item->setText(1, RzHexString(sdb_num_get(sdb, "Signature", 0)));
        ui->leftTreeWidget->addTopLevelItem(item);

        item = new QTreeWidgetItem();
        item->setText(0, "StrucVersion");
        item->setText(1, RzHexString(sdb_num_get(sdb, "StrucVersion", 0)));
        ui->leftTreeWidget->addTopLevelItem(item);

        item = new QTreeWidgetItem();
        item->setText(0, "FileVersion");
        item->setText(1, file_version);
        ui->leftTreeWidget->addTopLevelItem(item);

        item = new QTreeWidgetItem();
        item->setText(0, "ProductVersion");
        item->setText(1, product_version);
        ui->leftTreeWidget->addTopLevelItem(item);

        item = new QTreeWidgetItem();
        item->setText(0, "FileFlagsMask");
        item->setText(1, RzHexString(sdb_num_get(sdb, "FileFlagsMask", 0)));
        ui->leftTreeWidget->addTopLevelItem(item);

        item = new QTreeWidgetItem();
        item->setText(0, "FileFlags");
        item->setText(1, RzHexString(sdb_num_get(sdb, "FileFlags", 0)));
        ui->leftTreeWidget->addTopLevelItem(item);

        item = new QTreeWidgetItem();
        item->setText(0, "FileOS");
        item->setText(1, RzHexString(sdb_num_get(sdb, "FileOS", 0)));
        ui->leftTreeWidget->addTopLevelItem(item);

        item = new QTreeWidgetItem();
        item->setText(0, "FileType");
        item->setText(1, RzHexString(sdb_num_get(sdb, "FileType", 0)));
        ui->leftTreeWidget->addTopLevelItem(item);

        item = new QTreeWidgetItem();
        item->setText(0, "FileSubType");
        item->setText(1, RzHexString(sdb_num_get(sdb, "FileSubType", 0)));
        ui->leftTreeWidget->addTopLevelItem(item);

        free(file_version);
        free(product_version);

        // Adjust columns to content
        qhelpers::adjustColumns(ui->leftTreeWidget, 0);

        // Right tree
        for (int num_stringtable = 0;; num_stringtable++) {
            char *path_stringtable =
                    rz_str_newf("%s/string_file_info/stringtable%d", path_version, num_stringtable);
            sdb = sdb_ns_path(core->sdb, path_stringtable, 0);
            if (!sdb) {
                free(path_stringtable);
                break;
            }
            for (int num_string = 0; sdb; num_string++) {
                char *path_string = rz_str_newf("%s/string%d", path_stringtable, num_string);
                sdb = sdb_ns_path(core->sdb, path_string, 0);
                free(path_string);
                if (!sdb) {
                    continue;
                }
                int lenkey = 0;
                int lenval = 0;
                ut8 *key_utf16 = sdb_decode(sdb_const_get(sdb, "key", 0), &lenkey);
                ut8 *val_utf16 = sdb_decode(sdb_const_get(sdb, "value", 0), &lenval);
                item = new QTreeWidgetItem();
                item->setText(0, QString::fromUtf16(reinterpret_cast<const ushort *>(key_utf16)));
                item->setText(1, QString::fromUtf16(reinterpret_cast<const ushort *>(val_utf16)));
                ui->rightTreeWidget->addTopLevelItem(item);
                free(key_utf16);
                free(val_utf16);
            }
            free(path_stringtable);
        }
        free(path_version);
        qhelpers::adjustColumns(ui->rightTreeWidget, 0);
    }
}
