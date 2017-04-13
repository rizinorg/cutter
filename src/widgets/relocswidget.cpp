#include "relocswidget.h"
#include "ui_relocswidget.h"

#include "mainwindow.h"
#include "helpers.h"

#include <QTreeWidget>


RelocsWidget::RelocsWidget(MainWindow *main, QWidget *parent) :
    DockWidget(parent),
    ui(new Ui::RelocsWidget),
    main(main)
{
    ui->setupUi(this);

    ui->relocsTreeWidget->hideColumn(0);
}

RelocsWidget::~RelocsWidget()
{
    delete ui;
}

void RelocsWidget::setup()
{
    setScrollMode();

    fillTreeWidget();
}

void RelocsWidget::refresh()
{
    setup();
}

void RelocsWidget::on_relocsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    QNOTUSED(column);

    // Get offset and name of item double clicked
    // TODO: use this info to change disasm contents
    QString offset = item->text(1);
    QString name = item->text(2);
    main->seek(offset, name);

    main->raiseMemoryDock();
}

void RelocsWidget::fillTreeWidget()
{
    ui->relocsTreeWidget->clear();
    for (auto i : main->core->getList("bin", "relocs"))
    {
        QStringList pieces = i.split(",");
        if (pieces.length() == 3)
            qhelpers::appendRow(ui->relocsTreeWidget, pieces[0], pieces[1], pieces[2]);
    }
    qhelpers::adjustColumns(ui->relocsTreeWidget);
}

void RelocsWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->relocsTreeWidget);
}
