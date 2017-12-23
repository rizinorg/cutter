#include <QTreeWidget>
#include "RelocsWidget.h"
#include "ui_RelocsWidget.h"
#include "MainWindow.h"
#include "utils/Helpers.h"

RelocsWidget::RelocsWidget(MainWindow *main, QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::RelocsWidget),
    main(main)
{
    ui->setupUi(this);

    // Radare core found in:
    this->main = main;

    setScrollMode();

    connect(Core(), SIGNAL(refreshAll()), this, SLOT(fillTreeWidget()));
}

RelocsWidget::~RelocsWidget() {}

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
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, RAddressString(i.vaddr));
        item->setText(1, i.type);
        item->setText(2, i.name);
        item->setData(0, Qt::UserRole, QVariant::fromValue(i));
        ui->relocsTreeWidget->addTopLevelItem(item);
    }

    qhelpers::adjustColumns(ui->relocsTreeWidget, 0);
}

void RelocsWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->relocsTreeWidget);
}
