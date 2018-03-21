#ifndef SYMBOLSWIDGET_H
#define SYMBOLSWIDGET_H

#include <memory>

#include "CutterDockWidget.h"

class MainWindow;
class QTreeWidgetItem;

namespace Ui {
class SymbolsWidget;
}

class SymbolsWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit SymbolsWidget(MainWindow *main, QAction *action = nullptr);
    ~SymbolsWidget();

private slots:
    void on_symbolsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);

    void fillSymbols();

private:
    std::unique_ptr<Ui::SymbolsWidget> ui;

    void setScrollMode();
};

#endif // SYMBOLSWIDGET_H
