#include "SdbWidget.h"
#include "ui_SdbWidget.h"

#include "core/MainWindow.h"
#include "common/Helpers.h"

#include <QDebug>
#include <QTreeWidget>


SdbWidget::SdbWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::SdbWidget)
{
    ui->setupUi(this);

    path.clear();

    connect(Core(), SIGNAL(refreshAll()), this, SLOT(reload()));
    reload();
}

void SdbWidget::reload(QString _path)
{
    path = _path;

    ui->lineEdit->setText(path);
    /* insert root sdb keyvalue pairs */

    ui->treeWidget->clear();
    QList<QString> keys;
    /* key-values */
    keys = Core()->sdbListKeys(path);
    for (const QString &key : keys) {
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
    if (!path.isEmpty()) {
        keys.append("..");
    }
    for (const QString &key : keys) {
        QTreeWidgetItem *tempItem = new QTreeWidgetItem();
        tempItem->setText(0, key + "/");
        tempItem->setText(1, "");
        ui->treeWidget->insertTopLevelItem(0, tempItem);
    }
    qhelpers::adjustColumns(ui->treeWidget, 0);
}


void SdbWidget::on_treeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    if (column < 0)
        return;

    QString newpath;

    if (column == 0) {
        if (item->text(0) == "../") {
            int idx = path.lastIndexOf(QLatin1Char('/'));
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

SdbWidget::~SdbWidget() {}

void SdbWidget::on_lockButton_clicked()
{
    if (ui->lockButton->isChecked()) {
        this->setAllowedAreas(Qt::NoDockWidgetArea);
        ui->lockButton->setIcon(QIcon(":/lock"));
    } else {
        this->setAllowedAreas(Qt::AllDockWidgetAreas);
        ui->lockButton->setIcon(QIcon(":/unlock"));
    }
}

void SdbWidget::on_treeWidget_itemChanged(QTreeWidgetItem *item, int column)
{
    Core()->sdbSet(path, item->text(0), item->text(column));
}
