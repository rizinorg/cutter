#include "SdbDock.h"
#include "ui_SdbDock.h"

#include "MainWindow.h"
#include "utils/Helpers.h"

#include <QDebug>
#include <QTreeWidget>


SdbDock::SdbDock(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::SdbDock)
{
    ui->setupUi(this);

    path = "";

    connect(Core(), SIGNAL(refreshAll()), this, SLOT(reload()));
    reload(nullptr);
}

void SdbDock::reload(QString _path)
{
    if (!_path.isNull()) {
        path = _path;
    }

    ui->lineEdit->setText(path);
    /* insert root sdb keyvalue pairs */

    ui->treeWidget->clear();
    QList<QString> keys;
    /* key-values */
    keys = Core()->sdbListKeys(path);
    for (QString key : keys) {
        QTreeWidgetItem *tempItem = new QTreeWidgetItem();
        tempItem->setText(0, key);
        tempItem->setText(1, Core()->sdbGet(path, key));
        tempItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled |
                           Qt::ItemIsDragEnabled | Qt::ItemIsEditable);
        ui->treeWidget->insertTopLevelItem(0, tempItem);
    }
    qhelpers::adjustColumns(ui->treeWidget, 0);
    /* namespaces */
    keys = Core()->sdbList(path);
    keys.append("..");
    for (QString key : keys) {
        QTreeWidgetItem *tempItem = new QTreeWidgetItem();
        tempItem->setText(0, key + "/");
        tempItem->setText(1, "");
        ui->treeWidget->insertTopLevelItem(0, tempItem);
    }
    qhelpers::adjustColumns(ui->treeWidget, 0);
}


void SdbDock::on_treeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    if (column < 0)
        return;

    QString newpath;

    if (column == 0) {
        if (item->text(0) == "../") {
            int idx = path.lastIndexOf("/");
            if (idx != -1) {
                newpath = path.mid(0, idx);
            } else {
                newpath = "";
            }
            reload(newpath);

        } else if (item->text(0).indexOf("/") != -1) {
            if (path != "") {
                newpath = path + "/" + item->text(0).replace("/", "");
            } else {
                newpath = path + item->text(0).replace("/", "");
            }
            // enter directory
            reload(newpath);
        }
    }
}

SdbDock::~SdbDock() {}

void SdbDock::on_lockButton_clicked()
{
    if (ui->lockButton->isChecked()) {
        this->setAllowedAreas(Qt::NoDockWidgetArea);
        ui->lockButton->setIcon(QIcon(":/lock"));
    } else {
        this->setAllowedAreas(Qt::AllDockWidgetAreas);
        ui->lockButton->setIcon(QIcon(":/unlock"));
    }
}

void SdbDock::on_treeWidget_itemChanged(QTreeWidgetItem *item, int column)
{
    Core()->sdbSet(path, item->text(0), item->text(column));
}
