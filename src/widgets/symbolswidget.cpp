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
    // TODO: use this info to change disasm contents
    QString offset = item->text(1);
    QString name = item->text(3);
    //qDebug() << "Item Text: " << name;
    this->main->seek(offset, name);
    //ui->memDock->setWindowTitle(name);
}

void SymbolsWidget::fillSymbols()
{
    ui->symbolsTreeWidget->clear();
    for (auto i : this->main->core->getList("bin", "symbols"))
    {
        QStringList pieces = i.split(",");
        if (pieces.length() == 3)
            qhelpers::appendRow(ui->symbolsTreeWidget, pieces[0], pieces[1], pieces[2]);
    }
    qhelpers::adjustColumns(ui->symbolsTreeWidget);
}

void SymbolsWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->symbolsTreeWidget);
}
