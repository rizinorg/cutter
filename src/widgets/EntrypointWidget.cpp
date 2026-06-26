#include "EntrypointWidget.h"

#include "common/Helpers.h"
#include "core/MainWindow.h"
#include "ui_EntrypointWidget.h"

#include <QPen>
#include <QTreeWidget>

EntrypointWidget::EntrypointWidget(MainWindow *main)
    : CutterDockWidget(main), ui(new Ui::EntrypointWidget)
{
    ui->setupUi(this);

    setScrollMode();

    connect(Core(), &CutterCore::codeRebased, this, &EntrypointWidget::fillEntrypoint);
    connect(Core(), &CutterCore::refreshAll, this, &EntrypointWidget::fillEntrypoint);
    connect(ui->entrypointTreeWidget, &QTreeWidget::itemDoubleClicked, this,
            &EntrypointWidget::onEntrypointTreeWidgetItemDoubleClicked);
}

EntrypointWidget::~EntrypointWidget() {}

void EntrypointWidget::fillEntrypoint()
{
    ui->entrypointTreeWidget->clear();
    for (const EntrypointDescription &i : Core()->getAllEntrypoint()) {
        auto *item = new QTreeWidgetItem();
        item->setText(0, rzAddressString(i.vaddr));
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

void EntrypointWidget::onEntrypointTreeWidgetItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    if (column < 0) {
        return;
    }

    const auto ep = item->data(0, Qt::UserRole).value<EntrypointDescription>();
    Core()->seekAndShow(ep.vaddr);
}
