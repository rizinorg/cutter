#include "entrypointwidget.h"
#include "ui_entrypointwidget.h"

#include "mainwindow.h"
#include "helpers.h"

#include <QTreeWidget>
#include <QPen>


/*
 * Entrypoint Widget
 */

EntrypointWidget::EntrypointWidget(MainWindow *main, QWidget *parent) :
    DockWidget(parent),
    ui(new Ui::EntrypointWidget)
{
    ui->setupUi(this);

    // Radare core found in:
    this->main = main;

    // Delegate
    //CMyDelegate* delegate = new CMyDelegate(ui->importsTreeWidget);
    //ui->importsTreeWidget->setItemDelegate(delegate);

    ui->entrypointTreeWidget->hideColumn(0);
}

EntrypointWidget::~EntrypointWidget()
{
    delete ui;
}

void EntrypointWidget::setup()
{
    setScrollMode();

    fillEntrypoint();
}

void EntrypointWidget::refresh()
{
    setup();
}

void EntrypointWidget::fillEntrypoint()
{
    ui->entrypointTreeWidget->clear();
    for (auto i : this->main->core->getAllEntrypoint())
    {
        QTreeWidgetItem *item = qhelpers::appendRow(ui->entrypointTreeWidget, RAddressString(i.vaddr), i.type);
        item->setData(0, Qt::UserRole, QVariant::fromValue(i));
    }

    qhelpers::adjustColumns(ui->entrypointTreeWidget, 0, 10);
}

void EntrypointWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->entrypointTreeWidget);
}

void EntrypointWidget::on_entrypointTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int /* column */)
{
    EntrypointDescription ep = item->data(0, Qt::UserRole).value<EntrypointDescription>();
    this->main->seek(ep.vaddr, ep.type, true);
}
