#include "relocswidget.h"
#include "ui_relocswidget.h"

#include "mainwindow.h"

RelocsWidget::RelocsWidget(MainWindow *main, QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::RelocsWidget)
{
    ui->setupUi(this);

    // Radare core found in:
    this->main = main;
    this->relocsTreeWidget = ui->relocsTreeWidget;
}

RelocsWidget::~RelocsWidget()
{
    delete ui;
}

void RelocsWidget::on_relocsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    QNOTUSED(column);

    // Get offset and name of item double clicked
    // TODO: use this info to change disasm contents
    QString offset = item->text(1);
    QString name = item->text(2);
    main->seek(offset, name);
    this->main->memoryDock->raise();
}
