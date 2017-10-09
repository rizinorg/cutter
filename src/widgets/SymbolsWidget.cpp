#include "SymbolsWidget.h"
#include "ui_SymbolsWidget.h"

#include "MainWindow.h"
#include "utils/Helpers.h"

#include <QTreeWidget>


SymbolsWidget::SymbolsWidget(QWidget *parent) :
    DockWidget(parent),
    ui(new Ui::SymbolsWidget)
{
    ui->setupUi(this);

    ui->symbolsTreeWidget->hideColumn(0);
}

SymbolsWidget::~SymbolsWidget() {}

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
    Q_UNUSED(column);

    // Get offset and name of item double clicked
    SymbolDescription symbol = item->data(0, Qt::UserRole).value<SymbolDescription>();
    CutterCore::getInstance()->seek(symbol.vaddr);
}

void SymbolsWidget::fillSymbols()
{
    ui->symbolsTreeWidget->clear();
    for (auto symbol : CutterCore::getInstance()->getAllSymbols())
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
