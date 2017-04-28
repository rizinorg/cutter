#include "symbolswidget.h"
#include "ui_symbolswidget.h"

#include "mainwindow.h"
#include "helpers.h"

#include <QTreeWidget>


SymbolsWidget::SymbolsWidget(MainWindow *main, QWidget *parent) :
    DockWidget(parent),
    ui(new Ui::SymbolsWidget),
    main(main)
{
    ui->setupUi(this);

    ui->symbolsTreeWidget->hideColumn(0);
}

SymbolsWidget::~SymbolsWidget()
{
    delete ui;
}

void SymbolsWidget::setup()
{
    setScrollMode();

    fillSymbols();
}

void SymbolsWidget::refresh()
{
    setup();
}

void SymbolsWidget::on_symbolsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    QNOTUSED(column);

    // Get offset and name of item double clicked
    SymbolDescription symbol = item->data(0, Qt::UserRole).value<SymbolDescription>();
    this->main->seek(symbol.vaddr, symbol.name, true);
}

void SymbolsWidget::fillSymbols()
{
    ui->symbolsTreeWidget->clear();
    for (auto symbol : this->main->core->getAllSymbols())
    {
        QTreeWidgetItem *item = qhelpers::appendRow(ui->symbolsTreeWidget,
                                                      RAddressString(symbol.vaddr),
                                                      QString("%1 %2").arg(symbol.bind, symbol.type).trimmed(),
                                                      symbol.name);

        item->setData(0, Qt::UserRole, QVariant::fromValue(symbol));
    }
    qhelpers::adjustColumns(ui->symbolsTreeWidget);
}

void SymbolsWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->symbolsTreeWidget);
}
