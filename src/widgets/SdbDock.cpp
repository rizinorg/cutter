#include "SdbDock.h"
#include "ui_SdbDock.h"

#include "MainWindow.h"

#include <QDebug>
#include <QTreeWidget>


SdbDock::SdbDock(MainWindow *main, QWidget *parent) :
    DockWidget(parent),
    ui(new Ui::SdbDock)
{
    ui->setupUi(this);
    // Radare core found in:
    this->main = main;
    this->path = "";
    reload("");
}

void SdbDock::reload(QString path)
{
    ui->lineEdit->setText(path);
    this->path = path;
    /* insert root sdb keyvalue pairs */

    ui->treeWidget->clear();
    QList<QString> keys;
    /* key-values */
    keys = CutterCore::getInstance()->sdbListKeys(path);
    foreach (QString key, keys)
    {
        QTreeWidgetItem *tempItem = new QTreeWidgetItem();
        tempItem->setText(0, key);
        tempItem->setText(1, CutterCore::getInstance()->sdbGet(path, key));
        tempItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsEditable);
        ui->treeWidget->insertTopLevelItem(0, tempItem);
    }
    ui->treeWidget->resizeColumnToContents(0);
    ui->treeWidget->resizeColumnToContents(1);
    /* namespaces */
    keys = CutterCore::getInstance()->sdbList(path);
    keys.append("..");
    foreach (QString key, keys)
    {
        QTreeWidgetItem *tempItem = new QTreeWidgetItem();
        tempItem->setText(0, key + "/");
        tempItem->setText(1, "");
        ui->treeWidget->insertTopLevelItem(0, tempItem);
    }
    ui->treeWidget->resizeColumnToContents(0);
    ui->treeWidget->resizeColumnToContents(1);
}


void SdbDock::on_treeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    QString newpath;

    if (column == 0)
    {
        if (item->text(0) == "../")
        {
            int idx = path.lastIndexOf("/");
            if (idx != -1)
            {
                newpath = path.mid(0, idx);
            }
            else
            {
                newpath = "";
            }
            reload(newpath);

        }
        else if (item->text(0).indexOf("/") != -1)
        {
            if (path != "")
            {
                newpath = path + "/" + item->text(0).replace("/", "");
            }
            else
            {
                newpath = path + item->text(0).replace("/", "");
            }
            // enter directory
            reload(newpath);
        }
        else
        {
            //__alert ("TODO: change value");
        }
    }
}

SdbDock::~SdbDock() {}

void SdbDock::setup()
{
    // TODO: implement
    eprintf("%s - not implemented\n", Q_FUNC_INFO);
}

void SdbDock::refresh()
{
    // TODO: implement
    eprintf("%s - not implemented\n", Q_FUNC_INFO);
}

void SdbDock::on_lockButton_clicked()
{
    if (ui->lockButton->isChecked())
    {
        this->setAllowedAreas(Qt::NoDockWidgetArea);
        ui->lockButton->setIcon(QIcon(":/lock"));
    }
    else
    {
        this->setAllowedAreas(Qt::AllDockWidgetAreas);
        ui->lockButton->setIcon(QIcon(":/unlock"));
    }
}

void SdbDock::on_treeWidget_itemChanged(QTreeWidgetItem *item, int column)
{
    __alert(item->text(column));
    // El nuevo valor esta en:
    // item->text(column)
    // ya sabras tu que hacer con el :P
}
