#include "SymbolsWidget.h"
#include "ui_SymbolsWidget.h"

#include "MainWindow.h"
#include "utils/Helpers.h"

#include <QTreeWidget>


SymbolsWidget::SymbolsWidget(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::SymbolsWidget)
{
    ui->setupUi(this);

    ui->symbolsTreeWidget->sortByColumn(2, Qt::AscendingOrder);

    setScrollMode();

    connect(Core(), SIGNAL(refreshAll()), this, SLOT(fillSymbols()));
}

SymbolsWidget::~SymbolsWidget() {}


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
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, RAddressString(symbol.vaddr));
        item->setText(1, QString("%1 %2").arg(symbol.bind, symbol.type).trimmed());
        item->setText(2, symbol.name);
        item->setData(0, Qt::UserRole, QVariant::fromValue(symbol));
        ui->symbolsTreeWidget->addTopLevelItem(item);
    }
    qhelpers::adjustColumns(ui->symbolsTreeWidget, 0);
}

void SymbolsWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->symbolsTreeWidget);
}
