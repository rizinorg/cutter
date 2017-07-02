#include "stringswidget.h"
#include "ui_stringswidget.h"

//#include "dialogs/xrefsdialog.h"
#include "mainwindow.h"
#include "helpers.h"

#include <QTreeWidget>
#include <QAbstractItemView>


StringsWidget::StringsWidget(MainWindow *main, QWidget *parent) :
    DockWidget(parent),
    ui(new Ui::StringsWidget),
    main(main)
{
    ui->setupUi(this);

    ui->stringsTreeWidget->hideColumn(0);
}

StringsWidget::~StringsWidget()
{
    delete ui;
}

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
    IAITONOTUSED(column);

    // Get offset and name of item double clicked
    // TODO: use this info to change disasm contents
    StringDescription str = item->data(0, Qt::UserRole).value<StringDescription>();
    this->main->seek(str.vaddr, NULL, true);
}

void StringsWidget::fillTreeWidget()
{
    ui->stringsTreeWidget->clear();
    for (auto i : main->core->getAllStrings())
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
