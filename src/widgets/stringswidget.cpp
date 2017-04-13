#include "stringswidget.h"
#include "ui_stringswidget.h"

//#include "dialogs/xrefsdialog.h"
#include "mainwindow.h"
#include "helpers.h"

#include <QTreeWidget>


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
    QNOTUSED(column);

    // Get offset and name of item double clicked
    // TODO: use this info to change disasm contents
    QString offset = item->text(1);
    QString name = item->text(2);
    this->main->seek(offset);
    // Rise and shine baby!
    this->main->raiseMemoryDock();
}

void StringsWidget::fillTreeWidget()
{
    ui->stringsTreeWidget->clear();
    for (auto i : main->core->getList("bin", "strings"))
    {
        QStringList pieces = i.split(",");
        if (pieces.length() == 2)
            qhelpers::appendRow(ui->stringsTreeWidget, pieces[0], pieces[1]);
    }
    qhelpers::adjustColumns(ui->stringsTreeWidget);
}

void StringsWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->stringsTreeWidget);
}
