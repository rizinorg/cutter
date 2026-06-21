#include "VersionInfoDialog.h"

#include "Cutter.h"
#include "common/Helpers.h"
#include "ui_VersionInfoDialog.h"

#include <QClipboard>
#include <QContextMenuEvent>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <QTreeWidget>

VersionInfoDialog::VersionInfoDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::VersionInfoDialog)
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

    // Connect Copy actions
    connect(copyActionLeftTreewidget, &QAction::triggered, this,
            [this]() { copyTreeWidgetSelection(ui->leftTreeWidget); });

    connect(copyActionRightTreewidget, &QAction::triggered, this,
            [this]() { copyTreeWidgetSelection(ui->rightTreeWidget); });

    // Connect select sll actions
    connect(selAllActionLeftTreewidget, &QAction::triggered, this,
            [this]() { ui->leftTreeWidget->selectAll(); });

    connect(selAllActionRightTreewidget, &QAction::triggered, this,
            [this]() { ui->rightTreeWidget->selectAll(); });

    // Connect selection handles
    connect(ui->leftTreeWidget->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            [this]() { ui->rightTreeWidget->clearSelection(); });
    connect(ui->rightTreeWidget->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            [this]() { ui->leftTreeWidget->clearSelection(); });
    connect(this, &VersionInfoDialog::finished, this, &VersionInfoDialog::clearSelectionOnClose);

    connect(ui->copyVersionInfoButton, &QPushButton::clicked, this,
            &VersionInfoDialog::onCopyVersionInfoButtonClicked);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
            &VersionInfoDialog::onButtonBoxRejected);

    // Add actions to context menu
    ui->leftTreeWidget->addAction(copyActionLeftTreewidget);
    ui->leftTreeWidget->addAction(selAllActionLeftTreewidget);

    ui->rightTreeWidget->addAction(copyActionRightTreewidget);
    ui->rightTreeWidget->addAction(selAllActionRightTreewidget);
}

VersionInfoDialog::~VersionInfoDialog() {}

void VersionInfoDialog::clearSelectionOnClose()
{
    ui->leftTreeWidget->clearSelection();
    ui->rightTreeWidget->clearSelection();

    // remove default "current" item selection after dialog close
    const QModelIndex defaultIndex;
    ui->leftTreeWidget->setCurrentIndex(defaultIndex);
    ui->rightTreeWidget->setCurrentIndex(defaultIndex);
}

void VersionInfoDialog::copyTreeWidgetSelection(QTreeWidget *t)
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

void VersionInfoDialog::contextMenuEvent(QContextMenuEvent *event)
{
    contextMenu->exec(event->globalPos());
    event->accept();
}

void VersionInfoDialog::onButtonBoxRejected()
{
    close();
}

void VersionInfoDialog::onCopyVersionInfoButtonClicked()
{
    QString vinfo = "# " + ui->leftLabel->text() + "\n";

    // Iterate & Copy leftTreeWidget items
    QTreeWidgetItemIterator itl(ui->leftTreeWidget);

    const int keyColumnIndex = 0, valueColumnIndex = 1;

    while (*itl) {
        const QString row =
                (*itl)->text(keyColumnIndex) + " : " + (*itl)->text(valueColumnIndex) + "\n";
        vinfo.append(row);
        ++itl;
    }

    vinfo.append("\n# " + ui->rightLabel->text() + "\n");

    // Iterate & Copy rightTreeWidget items
    QTreeWidgetItemIterator itr(ui->rightTreeWidget);

    while (*itr) {
        const QString row =
                (*itr)->text(keyColumnIndex) + " : " + (*itr)->text(valueColumnIndex) + "\n";
        vinfo.append(row);
        ++itr;
    }

    // Copy to Clipboard
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(vinfo);

    QMessageBox::information(this, tr("Copy to Clipboard"),
                             tr("Version information was successfully copied!"));
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
        ui->leftLabel->setText(tr("Version symbols"));
        ui->rightLabel->setText(tr("Version need"));

        Sdb *sdb = sdb_ns_path(core->sdb, "bin/cur/info/versioninfo/versym", 0);
        if (!sdb) {
            return;
        }

        // Left tree
        auto *addrItemL = new QTreeWidgetItem();
        addrItemL->setText(0, tr("Address:"));
        addrItemL->setText(1, rzAddressString(sdb_num_get(sdb, "addr")));
        ui->leftTreeWidget->addTopLevelItem(addrItemL);

        auto *offItemL = new QTreeWidgetItem();
        offItemL->setText(0, tr("Offset:"));
        offItemL->setText(1, rzAddressString(sdb_num_get(sdb, "offset")));
        ui->leftTreeWidget->addTopLevelItem(offItemL);

        auto *entriesItemL = new QTreeWidgetItem();
        entriesItemL->setText(0, tr("Entries:"));
        const ut64 numEntries = sdb_num_get(sdb, "num_entries");
        for (size_t i = 0; i < numEntries; ++i) {
            auto key = QString("entry%0").arg(i);
            const char *const value = sdb_const_get(sdb, key.toStdString().c_str());
            if (!value) {
                continue;
            }
            auto item = new QTreeWidgetItem();
            item->setText(KeyColumn, rzAddressString(i));
            item->setText(ValueColumn, value);
            entriesItemL->addChild(item);
        }
        ui->leftTreeWidget->addTopLevelItem(entriesItemL);

        // Adjust columns to content
        qhelpers::adjustColumns(ui->leftTreeWidget, 0);
        sdb = sdb_ns_path(core->sdb, "bin/cur/info/versioninfo/verneed", 0);

        // Right tree
        auto *addrItemR = new QTreeWidgetItem();
        addrItemR->setText(0, tr("Address:"));
        addrItemR->setText(1, rzAddressString(sdb_num_get(sdb, "addr")));
        ui->rightTreeWidget->addTopLevelItem(addrItemR);

        auto *offItemR = new QTreeWidgetItem();
        offItemR->setText(0, tr("Offset:"));
        offItemR->setText(1, rzAddressString(sdb_num_get(sdb, "offset")));
        ui->rightTreeWidget->addTopLevelItem(offItemR);

        auto *entriesItemR = new QTreeWidgetItem();
        entriesItemR->setText(0, tr("Entries:"));
        for (size_t numVersion = 0;; numVersion++) {
            auto pathVersion =
                    QString("bin/cur/info/versioninfo/verneed/version%0").arg(numVersion);
            sdb = sdb_ns_path(core->sdb, pathVersion.toStdString().c_str(), 0);
            if (!sdb) {
                break;
            }
            const char *filename = sdb_const_get(sdb, "file_name");
            auto *parentItem = new QTreeWidgetItem();
            parentItem->setText(0, rzAddressString(sdb_num_get(sdb, "idx")));
            parentItem->setText(1,
                                tr("Version: %0\t"
                                   "File: %1")
                                        .arg(QString::number(sdb_num_get(sdb, "vn_version")),
                                             QString(filename)));

            int numVernaux = 0;
            while (true) {
                auto pathVernaux =
                        QString("%0/vernaux%1").arg(pathVersion, QString::number(numVernaux++));
                sdb = sdb_ns_path(core->sdb, pathVernaux.toStdString().c_str(), 0);
                if (!sdb) {
                    break;
                }

                auto *childItem = new QTreeWidgetItem();
                childItem->setText(0, rzAddressString(sdb_num_get(sdb, "idx")));
                const QString childString =
                        tr("Name: %0\t"
                           "Flags: %1\t"
                           "Version: %2\t")
                                .arg(sdb_const_get(sdb, "name"), sdb_const_get(sdb, "flags"),
                                     QString::number(sdb_num_get(sdb, "version")));
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
        ui->leftLabel->setText(tr("VS Fixed file info"));
        ui->rightLabel->setText(tr("String table"));
        Sdb *sdb = nullptr;

        // Left tree
        auto pathVersion = QString("bin/cur/info/vs_version_info/VS_VERSIONINFO%0").arg(0);
        auto pathFixedfileinfo = QString("%0/fixed_file_info").arg(pathVersion);
        sdb = sdb_ns_path(core->sdb, pathFixedfileinfo.toStdString().c_str(), 0);
        if (!sdb) {
            return;
        }
        const ut32 fileVersionMs = sdb_num_get(sdb, "FileVersionMS");
        const ut32 fileVersionLs = sdb_num_get(sdb, "FileVersionLS");
        auto fileVersion = QString("%0.%1.%2.%3")
                                   .arg(fileVersionMs >> 16)
                                   .arg(fileVersionMs & 0xFFFF)
                                   .arg(fileVersionLs >> 16)
                                   .arg(fileVersionLs & 0xFFFF);
        const ut32 productVersionMs = sdb_num_get(sdb, "ProductVersionMS");
        const ut32 productVersionLs = sdb_num_get(sdb, "ProductVersionLS");
        auto productVersion = QString("%0.%1.%2.%3")
                                      .arg(productVersionMs >> 16)
                                      .arg(productVersionMs & 0xFFFF)
                                      .arg(productVersionLs >> 16)
                                      .arg(productVersionLs & 0xFFFF);

        auto item = new QTreeWidgetItem();
        item->setText(0, "Signature");
        item->setText(1, rzHexString(sdb_num_get(sdb, "Signature")));
        ui->leftTreeWidget->addTopLevelItem(item);

        item = new QTreeWidgetItem();
        item->setText(0, "StrucVersion");
        item->setText(1, rzHexString(sdb_num_get(sdb, "StrucVersion")));
        ui->leftTreeWidget->addTopLevelItem(item);

        item = new QTreeWidgetItem();
        item->setText(KeyColumn, "FileVersion");
        item->setText(ValueColumn, fileVersion);
        ui->leftTreeWidget->addTopLevelItem(item);

        item = new QTreeWidgetItem();
        item->setText(KeyColumn, "ProductVersion");
        item->setText(ValueColumn, productVersion);
        ui->leftTreeWidget->addTopLevelItem(item);

        item = new QTreeWidgetItem();
        item->setText(0, "FileFlagsMask");
        item->setText(1, rzHexString(sdb_num_get(sdb, "FileFlagsMask")));
        ui->leftTreeWidget->addTopLevelItem(item);

        item = new QTreeWidgetItem();
        item->setText(0, "FileFlags");
        item->setText(1, rzHexString(sdb_num_get(sdb, "FileFlags")));
        ui->leftTreeWidget->addTopLevelItem(item);

        item = new QTreeWidgetItem();
        item->setText(0, "FileOS");
        item->setText(1, rzHexString(sdb_num_get(sdb, "FileOS")));
        ui->leftTreeWidget->addTopLevelItem(item);

        item = new QTreeWidgetItem();
        item->setText(0, "FileType");
        item->setText(1, rzHexString(sdb_num_get(sdb, "FileType")));
        ui->leftTreeWidget->addTopLevelItem(item);

        item = new QTreeWidgetItem();
        item->setText(0, "FileSubType");
        item->setText(1, rzHexString(sdb_num_get(sdb, "FileSubType")));
        ui->leftTreeWidget->addTopLevelItem(item);

        // Adjust columns to content
        qhelpers::adjustColumns(ui->leftTreeWidget, 0);

        // Right tree
        for (int numStringtable = 0;; numStringtable++) {
            auto pathStringtable = QString("%0/string_file_info/stringtable%1")
                                           .arg(pathVersion)
                                           .arg(numStringtable);
            sdb = sdb_ns_path(core->sdb, pathStringtable.toStdString().c_str(), 0);
            if (!sdb) {
                break;
            }
            for (int numString = 0; sdb; numString++) {
                auto pathString = QString("%0/string%1").arg(pathStringtable).arg(numString);
                sdb = sdb_ns_path(core->sdb, pathString.toStdString().c_str(), 0);
                if (!sdb) {
                    continue;
                }
                int lenkey = 0;
                int lenval = 0;
                ut8 *keyUtf16 = sdb_decode(sdb_const_get(sdb, "key"), &lenkey);
                ut8 *valUtf16 = sdb_decode(sdb_const_get(sdb, "value"), &lenval);
                item = new QTreeWidgetItem();
                item->setText(KeyColumn,
                              QString::fromUtf16(reinterpret_cast<const char16_t *>(keyUtf16)));
                item->setText(ValueColumn,
                              QString::fromUtf16(reinterpret_cast<const char16_t *>(valUtf16)));
                ui->rightTreeWidget->addTopLevelItem(item);
                free(keyUtf16);
                free(valUtf16);
            }
        }
        qhelpers::adjustColumns(ui->rightTreeWidget, 0);
    }
}
