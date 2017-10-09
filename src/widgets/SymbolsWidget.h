#ifndef SYMBOLSWIDGET_H
#define SYMBOLSWIDGET_H

#include "DockWidget.h"
#include <memory>

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
    explicit SymbolsWidget(QWidget *parent = 0);
    ~SymbolsWidget();

    void setup() override;

    void refresh() override;

private slots:
    void on_symbolsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);

private:
    std::unique_ptr<Ui::SymbolsWidget> ui;

    void fillSymbols();
    void setScrollMode();
};

#endif // SYMBOLSWIDGET_H
