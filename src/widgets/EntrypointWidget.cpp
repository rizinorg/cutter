#include "EntrypointWidget.h"
#include "ui_EntrypointWidget.h"

#include "MainWindow.h"
#include "utils/Helpers.h"

#include <QTreeWidget>
#include <QPen>


/*
 * Entrypoint Widget
 */

EntrypointWidget::EntrypointWidget(MainWindow *main, QWidget *parent) :
    DockWidget(parent),
    ui(new Ui::EntrypointWidget),
    main(main)
{
    ui->setupUi(this);

    // Delegate
    //CMyDelegate* delegate = new CMyDelegate(ui->importsTreeWidget);
    //ui->importsTreeWidget->setItemDelegate(delegate);

    ui->entrypointTreeWidget->hideColumn(0);
}

EntrypointWidget::~EntrypointWidget() {}

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
    for (auto i : CutterCore::getInstance()->getAllEntrypoint())
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
    CutterCore::getInstance()->seek(ep.vaddr);
    //CutterCore::getInstance()->seek(ep.vaddr, ep.type, true);
}
