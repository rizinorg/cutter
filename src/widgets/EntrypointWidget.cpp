#include "EntrypointWidget.h"
#include "ui_EntrypointWidget.h"

#include "core/MainWindow.h"
#include "common/Helpers.h"

#include <QTreeWidget>
#include <QPen>


/*
 * Entrypoint Widget
 */

EntrypointWidget::EntrypointWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::EntrypointWidget)
{
    ui->setupUi(this);

    setScrollMode();

    connect(Core(), &CutterCore::codeRebased, this, &EntrypointWidget::fillEntrypoint);
    connect(Core(), &CutterCore::refreshAll, this, &EntrypointWidget::fillEntrypoint);
}

EntrypointWidget::~EntrypointWidget() {}

void EntrypointWidget::fillEntrypoint()
{
    ui->entrypointTreeWidget->clear();
    for (const EntrypointDescription &i : Core()->getAllEntrypoint()) {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, RAddressString(i.vaddr));
        item->setText(1, i.type);
        item->setData(0, Qt::UserRole, QVariant::fromValue(i));
        ui->entrypointTreeWidget->addTopLevelItem(item);
    }

    qhelpers::adjustColumns(ui->entrypointTreeWidget, 0, 10);
}

void EntrypointWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->entrypointTreeWidget);
}

void EntrypointWidget::on_entrypointTreeWidget_itemDoubleClicked(QTreeWidgetItem *item,
                                                                 int column)
{
    if (column < 0)
        return;

    EntrypointDescription ep = item->data(0, Qt::UserRole).value<EntrypointDescription>();
    Core()->seekAndShow(ep.vaddr);
}
