#include <QTreeWidget>
#include "RelocsWidget.h"
#include "ui_RelocsWidget.h"
#include "MainWindow.h"
#include "utils/Helpers.h"

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

RelocsWidget::~RelocsWidget() {}

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
    Q_UNUSED(column);

    // Get offset and name of item double clicked
    RelocDescription reloc = item->data(0, Qt::UserRole).value<RelocDescription>();
    CutterCore::getInstance()->seek(reloc.vaddr);
}

void RelocsWidget::fillTreeWidget()
{
    ui->relocsTreeWidget->clear();

    for (auto i : CutterCore::getInstance()->getAllRelocs())
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
