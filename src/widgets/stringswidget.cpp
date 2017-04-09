#include "stringswidget.h"
#include "ui_stringswidget.h"

#include "dialogs/xrefsdialog.h"
#include "mainwindow.h"

StringsWidget::StringsWidget(MainWindow *main, QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::StringsWidget)
{
    ui->setupUi(this);

    // Radare core found in:
    this->main = main;
    this->stringsTreeWidget = ui->stringsTreeWidget;
}

StringsWidget::~StringsWidget()
{
    delete ui;
}

void StringsWidget::on_stringsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    QNOTUSED(column);

    // Get offset and name of item double clicked
    // TODO: use this info to change disasm contents
    QString offset = item->text(1);
    QString name = item->text(2);
    this->main->seek (offset);
    // Rise and shine baby!
    this->main->memoryDock->raise();
}
