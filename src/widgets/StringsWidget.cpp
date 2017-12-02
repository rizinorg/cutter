#include <QTreeWidget>
#include <QAbstractItemView>
#include "StringsWidget.h"
#include "ui_StringsWidget.h"

#include "MainWindow.h"
#include "utils/Helpers.h"


StringsWidget::StringsWidget(MainWindow *main, QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::StringsWidget),
    main(main)
{
    ui->setupUi(this);

    setScrollMode();

    ui->stringsTreeWidget->sortByColumn(1, Qt::AscendingOrder);

    connect(Core(), SIGNAL(refreshAll()), this, SLOT(fillTreeWidget()));
}

StringsWidget::~StringsWidget() {}

void StringsWidget::on_stringsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    StringDescription str = item->data(0, Qt::UserRole).value<StringDescription>();
    CutterCore::getInstance()->seek(str.vaddr);
}

void StringsWidget::fillTreeWidget()
{
    ui->stringsTreeWidget->clear();
    for (auto i : CutterCore::getInstance()->getAllStrings())
    {
        QTreeWidgetItem *item = new QTreeWidgetItem();

        item->setText(0, RAddressString(i.vaddr));
        item->setText(1, i.string);

        ui->stringsTreeWidget->insertTopLevelItem(0, item);

        item->setData(0, Qt::UserRole, QVariant::fromValue(i));
    }
    qhelpers::adjustColumns(ui->stringsTreeWidget);
}

void StringsWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->stringsTreeWidget);
}
