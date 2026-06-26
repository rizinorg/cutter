#include "SdbWidget.h"

#include "common/Helpers.h"
#include "core/MainWindow.h"
#include "ui_SdbWidget.h"

#include <QDebug>
#include <QTreeWidget>

#include <utility>

SdbWidget::SdbWidget(MainWindow *main) : CutterDockWidget(main), ui(new Ui::SdbWidget)
{
    ui->setupUi(this);

    path.clear();

    connect(Core(), &CutterCore::refreshAll, this, [this]() { reload(); });

    connect(ui->treeWidget, &QTreeWidget::itemDoubleClicked, this,
            &SdbWidget::onTreeWidgetItemDoubleClicked);
    connect(ui->treeWidget, &QTreeWidget::itemChanged, this, &SdbWidget::onTreeWidgetItemChanged);
    connect(ui->lockButton, &QToolButton::clicked, this, &SdbWidget::onLockButtonClicked);

    reload();
}

void SdbWidget::reload(QString _path)
{
    path = std::move(_path);

    ui->lineEdit->setText(path);
    /* insert root sdb keyvalue pairs */

    ui->treeWidget->clear();
    QList<QString> keys;
    /* key-values */
    keys = Core()->sdbListKeys(path);
    for (const QString &key : std::as_const(keys)) {
        auto *tempItem = new QTreeWidgetItem();
        tempItem->setText(0, key);
        tempItem->setText(1, Core()->sdbGet(path, key));
        tempItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled
                           | Qt::ItemIsDragEnabled | Qt::ItemIsEditable);
        ui->treeWidget->insertTopLevelItem(0, tempItem);
    }
    qhelpers::adjustColumns(ui->treeWidget, 0);
    /* namespaces */
    keys = Core()->sdbList(path);
    if (!path.isEmpty()) {
        keys.append("..");
    }
    for (const QString &key : std::as_const(keys)) {
        auto *tempItem = new QTreeWidgetItem();
        tempItem->setText(0, key + "/");
        tempItem->setText(1, "");
        ui->treeWidget->insertTopLevelItem(0, tempItem);
    }
    qhelpers::adjustColumns(ui->treeWidget, 0);
}

void SdbWidget::onTreeWidgetItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    if (column < 0) {
        return;
    }

    QString newpath;

    if (column == 0) {
        if (item->text(0) == "../") {
            const int idx = path.lastIndexOf(QLatin1Char('/'));
            if (idx != -1) {
                newpath = path.mid(0, idx);
            } else {
                newpath.clear();
            }
            reload(newpath);

        } else if (item->text(0).indexOf(QLatin1Char('/')) != -1) {
            if (!path.isEmpty()) {
                newpath = path + "/" + item->text(0).remove(QLatin1Char('/'));
            } else {
                newpath = path + item->text(0).remove(QLatin1Char('/'));
            }
            // enter directory
            reload(newpath);
        }
    }
}

SdbWidget::~SdbWidget() = default;

void SdbWidget::onLockButtonClicked()
{
    if (ui->lockButton->isChecked()) {
        this->setAllowedAreas(Qt::NoDockWidgetArea);
        ui->lockButton->setIcon(QIcon(":/lock"));
    } else {
        this->setAllowedAreas(Qt::AllDockWidgetAreas);
        ui->lockButton->setIcon(QIcon(":/unlock"));
    }
}

void SdbWidget::onTreeWidgetItemChanged(QTreeWidgetItem *item, int column)
{
    Core()->sdbSet(path, item->text(0), item->text(column));
}
