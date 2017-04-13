#ifndef SYMBOLSWIDGET_H
#define SYMBOLSWIDGET_H

#include "dockwidget.h"

class MainWindow;
class QTreeWidgetItem;

namespace Ui
{
    class SymbolsWidget;
}

class SymbolsWidget : public DockWidget
{
    Q_OBJECT

public:
    explicit SymbolsWidget(MainWindow *main, QWidget *parent = 0);
    ~SymbolsWidget();

    void setup() override;

    void refresh() override;

private slots:
    void on_symbolsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);

private:
    Ui::SymbolsWidget *ui;
    MainWindow      *main;

    void fillSymbols();
    void setScrollMode();
};

#endif // SYMBOLSWIDGET_H
