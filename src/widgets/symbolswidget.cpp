#include "symbolswidget.h"
#include "ui_symbolswidget.h"

#include "mainwindow.h"

SymbolsWidget::SymbolsWidget(MainWindow *main, QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::SymbolsWidget)
{
    ui->setupUi(this);
    this->main = main;
    this->symbolsTreeWidget = ui->symbolsTreeWidget;
}

SymbolsWidget::~SymbolsWidget()
{
    delete ui;
}

void SymbolsWidget::fillSymbols() {
    this->symbolsTreeWidget->clear();
    for (auto i: this->main->core->getList ("bin", "symbols")) {
        QStringList pieces = i.split (",");
        if (pieces.length()==3)
            this->main->appendRow(this->symbolsTreeWidget, pieces[0], pieces[1], pieces[2]);
    }
    this->main->adjustColumns(this->symbolsTreeWidget);
}

void SymbolsWidget::on_symbolsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    QNOTUSED(column);

    // Get offset and name of item double clicked
    // TODO: use this info to change disasm contents
    QString offset = item->text(1);
    QString name = item->text(3);
    //qDebug() << "Item Text: " << name;
    this->main->seek(offset, name);
    //ui->memDock->setWindowTitle(name);
}
