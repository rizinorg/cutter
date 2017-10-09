#include <QTreeWidget>
#include <QAbstractItemView>
#include "StringsWidget.h"
#include "ui_StringsWidget.h"

#include "MainWindow.h"
#include "utils/Helpers.h"


StringsWidget::StringsWidget(MainWindow *main, QWidget *parent) :
    DockWidget(parent),
    ui(new Ui::StringsWidget),
    main(main)
{
    ui->setupUi(this);

    ui->stringsTreeWidget->hideColumn(0);
}

StringsWidget::~StringsWidget() {}

void StringsWidget::setup()
{
    setScrollMode();

    fillTreeWidget();
}

void StringsWidget::refresh()
{
    setup();
}

void StringsWidget::on_stringsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    // Get offset and name of item double clicked
    // TODO: use this info to change disasm contents
    StringDescription str = item->data(0, Qt::UserRole).value<StringDescription>();
    CutterCore::getInstance()->seek(str.vaddr);
}

void StringsWidget::fillTreeWidget()
{
    ui->stringsTreeWidget->clear();
    for (auto i : CutterCore::getInstance()->getAllStrings())
    {
        QTreeWidgetItem *item = qhelpers::appendRow(ui->stringsTreeWidget, RAddressString(i.vaddr), i.string);
        item->setData(0, Qt::UserRole, QVariant::fromValue(i));
    }
    qhelpers::adjustColumns(ui->stringsTreeWidget);
}

void StringsWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->stringsTreeWidget);
}
