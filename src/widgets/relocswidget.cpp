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

    // Radare core found in:
    this->main = main;

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
    IAITONOTUSED(column);

    // Get offset and name of item double clicked
    RelocDescription reloc = item->data(0, Qt::UserRole).value<RelocDescription>();
    main->seek(reloc.vaddr, reloc.name, true);
}

void RelocsWidget::fillTreeWidget()
{
    ui->relocsTreeWidget->clear();

    for (auto i : main->core->getAllRelocs())
    {
        QTreeWidgetItem *item = qhelpers::appendRow(ui->relocsTreeWidget, RAddressString(i.vaddr), i.type, i.name);
        item->setData(0, Qt::UserRole, QVariant::fromValue(i));
    }

    qhelpers::adjustColumns(ui->relocsTreeWidget);
}

void RelocsWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->relocsTreeWidget);
}
