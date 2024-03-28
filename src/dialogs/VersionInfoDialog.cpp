#include "VersionInfoDialog.h"
#include "ui_VersionInfoDialog.h"

#include "common/Helpers.h"

#include <QJsonArray>
#include <QStringList>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTreeWidget>
#include <QContextMenuEvent>
#include <QClipboard>

VersionInfoDialog::VersionInfoDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::VersionInfoDialog), core(Core())
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

    // Get version information
    fillVersionInfo();

    // Setup context menu and actions
    copyActionLeftTreewidget = new QAction(tr("Copy"), this);
    copyActionLeftTreewidget->setIcon(QIcon(":/img/icons/copy.svg"));
    copyActionLeftTreewidget->setShortcut(QKeySequence::StandardKey::Copy);
    copyActionLeftTreewidget->setShortcutContext(Qt::ShortcutContext::WidgetWithChildrenShortcut);

    copyActionRightTreewidget = new QAction(tr("Copy"), this);
    copyActionRightTreewidget->setIcon(QIcon(":/img/icons/copy.svg"));
    copyActionRightTreewidget->setShortcut(QKeySequence::StandardKey::Copy);
    copyActionRightTreewidget->setShortcutContext(Qt::ShortcutContext::WidgetWithChildrenShortcut);

    selAllActionLeftTreewidget = new QAction(tr("Select All"), this);
    selAllActionLeftTreewidget->setShortcut(QKeySequence::StandardKey::SelectAll);
    selAllActionLeftTreewidget->setShortcutContext(Qt::ShortcutContext::WidgetWithChildrenShortcut);

    selAllActionRightTreewidget = new QAction(tr("Select All"), this);
    selAllActionRightTreewidget->setShortcut(QKeySequence::StandardKey::SelectAll);
    selAllActionRightTreewidget->setShortcutContext(
            Qt::ShortcutContext::WidgetWithChildrenShortcut);

    connect(copyActionLeftTreewidget, &QAction::triggered, this,
            [this]() { CopyTreeWidgetSelection(ui->leftTreeWidget); });

    connect(copyActionRightTreewidget, &QAction::triggered, this,
            [this]() { CopyTreeWidgetSelection(ui->rightTreeWidget); });

    connect(selAllActionLeftTreewidget, &QAction::triggered, this,
            [this]() { ui->leftTreeWidget->selectAll(); });

    connect(selAllActionRightTreewidget, &QAction::triggered, this,
            [this]() { ui->rightTreeWidget->selectAll(); });

    ui->leftTreeWidget->addAction(copyActionLeftTreewidget);
    ui->leftTreeWidget->addAction(selAllActionLeftTreewidget);

    ui->rightTreeWidget->addAction(copyActionRightTreewidget);
    ui->rightTreeWidget->addAction(selAllActionRightTreewidget);
}

VersionInfoDialog::~VersionInfoDialog() {}

void VersionInfoDialog::CopyTreeWidgetSelection(QTreeWidget *t)
{
    QString vinfo, row;

    QTreeWidgetItemIterator it(t);

    while (*it) {
        if ((*it)->isSelected()) {
            row = (*it)->text(KeyColumn) + " " + (*it)->text(ValueColumn) + "\n";
            vinfo.append(row);
        }
        it++;
    }

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(vinfo.trimmed());
}

void VersionInfoDialog::fillVersionInfo()
{
    RzCoreLocked core(Core());
    RzBinObject *bobj = rz_bin_cur_object(core->bin);
    if (!bobj) {
        return;
    }
    const RzBinInfo *info = rz_bin_object_get_info(bobj);
    if (!info || !info->rclass) {
        return;
    }
    // Case ELF
    if (strncmp("elf", info->rclass, 3) == 0) {
        // Set labels
        ui->leftLabel->setText("Version symbols");
        ui->rightLabel->setText("Version need");

        Sdb *sdb = sdb_ns_path(core->sdb, "bin/cur/info/versioninfo/versym", 0);
        if (!sdb) {
            return;
        }

        // Left tree
        QTreeWidgetItem *addrItemL = new QTreeWidgetItem();
        addrItemL->setText(KeyColumn, "Address:");
        addrItemL->setText(ValueColumn, RzAddressString(sdb_num_get(sdb, "addr", 0)));
        ui->leftTreeWidget->addTopLevelItem(addrItemL);

        QTreeWidgetItem *offItemL = new QTreeWidgetItem();
        offItemL->setText(KeyColumn, "Offset:");
        offItemL->setText(ValueColumn, RzAddressString(sdb_num_get(sdb, "offset", 0)));
        ui->leftTreeWidget->addTopLevelItem(offItemL);

        QTreeWidgetItem *entriesItemL = new QTreeWidgetItem();
        entriesItemL->setText(KeyColumn, "Entries:");
        const ut64 num_entries = sdb_num_get(sdb, "num_entries", 0);
        for (size_t i = 0; i < num_entries; ++i) {
            auto key = QString("entry%0").arg(i);
            const char *const value = sdb_const_get(sdb, key.toStdString().c_str(), 0);
            if (!value) {
                continue;
            }
            auto item = new QTreeWidgetItem();
            item->setText(KeyColumn, RzAddressString(i));
            item->setText(ValueColumn, value);
            entriesItemL->addChild(item);
        }
        ui->leftTreeWidget->addTopLevelItem(entriesItemL);

        // Adjust columns to content
        qhelpers::adjustColumns(ui->leftTreeWidget, 0);
        sdb = sdb_ns_path(core->sdb, "bin/cur/info/versioninfo/verneed", 0);

        // Right tree
        QTreeWidgetItem *addrItemR = new QTreeWidgetItem();
        addrItemR->setText(KeyColumn, "Address:");
        addrItemR->setText(ValueColumn, RzAddressString(sdb_num_get(sdb, "addr", 0)));
        ui->rightTreeWidget->addTopLevelItem(addrItemR);

        QTreeWidgetItem *offItemR = new QTreeWidgetItem();
        offItemR->setText(KeyColumn, "Offset:");
        offItemR->setText(ValueColumn, RzAddressString(sdb_num_get(sdb, "offset", 0)));
        ui->rightTreeWidget->addTopLevelItem(offItemR);

        QTreeWidgetItem *entriesItemR = new QTreeWidgetItem();
        entriesItemR->setText(KeyColumn, "Entries:");
        for (size_t num_version = 0;; num_version++) {
            auto path_version =
                    QString("bin/cur/info/versioninfo/verneed/version%0").arg(num_version);
            sdb = sdb_ns_path(core->sdb, path_version.toStdString().c_str(), 0);
            if (!sdb) {
                break;
            }
            const char *filename = sdb_const_get(sdb, "file_name", 0);
            auto *parentItem = new QTreeWidgetItem();
            parentItem->setText(KeyColumn, RzAddressString(sdb_num_get(sdb, "idx", 0)));
            parentItem->setText(ValueColumn,
                                QString("Version: %0\t"
                                        "File: %1")
                                        .arg(QString::number(sdb_num_get(sdb, "vn_version", 0)),
                                             QString(filename)));

            int num_vernaux = 0;
            while (true) {
                auto path_vernaux =
                        QString("%0/vernaux%1").arg(path_version, QString::number(num_vernaux++));
                sdb = sdb_ns_path(core->sdb, path_vernaux.toStdString().c_str(), 0);
                if (!sdb) {
                    break;
                }

                auto *childItem = new QTreeWidgetItem();
                childItem->setText(KeyColumn, RzAddressString(sdb_num_get(sdb, "idx", 0)));
                QString childString =
                        QString("Name: %0\t"
                                "Flags: %1\t"
                                "Version: %2\t")
                                .arg(sdb_const_get(sdb, "name", 0), sdb_const_get(sdb, "flags", 0),
                                     QString::number(sdb_num_get(sdb, "version", 0)));
                childItem->setText(ValueColumn, childString);
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
        auto path_version = QString("bin/cur/info/vs_version_info/VS_VERSIONINFO%0").arg(0);
        auto path_fixedfileinfo = QString("%0/fixed_file_info").arg(path_version);
        sdb = sdb_ns_path(core->sdb, path_fixedfileinfo.toStdString().c_str(), 0);
        if (!sdb) {
            return;
        }
        ut32 file_version_ms = sdb_num_get(sdb, "FileVersionMS", 0);
        ut32 file_version_ls = sdb_num_get(sdb, "FileVersionLS", 0);
        auto file_version = QString("%0.%1.%2.%3")
                                    .arg(file_version_ms >> 16)
                                    .arg(file_version_ms & 0xFFFF)
                                    .arg(file_version_ls >> 16)
                                    .arg(file_version_ls & 0xFFFF);
        ut32 product_version_ms = sdb_num_get(sdb, "ProductVersionMS", 0);
        ut32 product_version_ls = sdb_num_get(sdb, "ProductVersionLS", 0);
        auto product_version = QString("%0.%1.%2.%3")
                                       .arg(product_version_ms >> 16)
                                       .arg(product_version_ms & 0xFFFF)
                                       .arg(product_version_ls >> 16)
                                       .arg(product_version_ls & 0xFFFF);

        auto item = new QTreeWidgetItem();
        item->setText(KeyColumn, "Signature");
        item->setText(ValueColumn, RzHexString(sdb_num_get(sdb, "Signature", 0)));
        ui->leftTreeWidget->addTopLevelItem(item);

        item = new QTreeWidgetItem();
        item->setText(KeyColumn, "StrucVersion");
        item->setText(ValueColumn, RzHexString(sdb_num_get(sdb, "StrucVersion", 0)));
        ui->leftTreeWidget->addTopLevelItem(item);

        item = new QTreeWidgetItem();
        item->setText(KeyColumn, "FileVersion");
        item->setText(ValueColumn, file_version);
        ui->leftTreeWidget->addTopLevelItem(item);

        item = new QTreeWidgetItem();
        item->setText(KeyColumn, "ProductVersion");
        item->setText(ValueColumn, product_version);
        ui->leftTreeWidget->addTopLevelItem(item);

        item = new QTreeWidgetItem();
        item->setText(KeyColumn, "FileFlagsMask");
        item->setText(ValueColumn, RzHexString(sdb_num_get(sdb, "FileFlagsMask", 0)));
        ui->leftTreeWidget->addTopLevelItem(item);

        item = new QTreeWidgetItem();
        item->setText(KeyColumn, "FileFlags");
        item->setText(ValueColumn, RzHexString(sdb_num_get(sdb, "FileFlags", 0)));
        ui->leftTreeWidget->addTopLevelItem(item);

        item = new QTreeWidgetItem();
        item->setText(KeyColumn, "FileOS");
        item->setText(ValueColumn, RzHexString(sdb_num_get(sdb, "FileOS", 0)));
        ui->leftTreeWidget->addTopLevelItem(item);

        item = new QTreeWidgetItem();
        item->setText(KeyColumn, "FileType");
        item->setText(ValueColumn, RzHexString(sdb_num_get(sdb, "FileType", 0)));
        ui->leftTreeWidget->addTopLevelItem(item);

        item = new QTreeWidgetItem();
        item->setText(KeyColumn, "FileSubType");
        item->setText(ValueColumn, RzHexString(sdb_num_get(sdb, "FileSubType", 0)));
        ui->leftTreeWidget->addTopLevelItem(item);

        // Adjust columns to content
        qhelpers::adjustColumns(ui->leftTreeWidget, 0);

        // Right tree
        for (int num_stringtable = 0;; num_stringtable++) {
            auto path_stringtable = QString("%0/string_file_info/stringtable%1")
                                            .arg(path_version)
                                            .arg(num_stringtable);
            sdb = sdb_ns_path(core->sdb, path_stringtable.toStdString().c_str(), 0);
            if (!sdb) {
                break;
            }
            for (int num_string = 0; sdb; num_string++) {
                auto path_string = QString("%0/string%1").arg(path_stringtable).arg(num_string);
                sdb = sdb_ns_path(core->sdb, path_string.toStdString().c_str(), 0);
                if (!sdb) {
                    continue;
                }
                int lenkey = 0;
                int lenval = 0;
                ut8 *key_utf16 = sdb_decode(sdb_const_get(sdb, "key", 0), &lenkey);
                ut8 *val_utf16 = sdb_decode(sdb_const_get(sdb, "value", 0), &lenval);
                item = new QTreeWidgetItem();
                item->setText(KeyColumn,
                              QString::fromUtf16(reinterpret_cast<const ushort *>(key_utf16)));
                item->setText(ValueColumn,
                              QString::fromUtf16(reinterpret_cast<const ushort *>(val_utf16)));
                ui->rightTreeWidget->addTopLevelItem(item);
                free(key_utf16);
                free(val_utf16);
            }
        }
        qhelpers::adjustColumns(ui->rightTreeWidget, 0);
    }
}