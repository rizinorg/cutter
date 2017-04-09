#ifndef SYMBOLSWIDGET_H
#define SYMBOLSWIDGET_H

#include <QDockWidget>
#include <QTreeWidget>

class MainWindow;

namespace Ui
{
    class SymbolsWidget;
}

class SymbolsWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit SymbolsWidget(MainWindow *main, QWidget *parent = 0);
    ~SymbolsWidget();

    QTreeWidget    *symbolsTreeWidget;
    void fillSymbols();

private slots:
    void on_symbolsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);

private:
    Ui::SymbolsWidget *ui;

    MainWindow      *main;
};

#endif // SYMBOLSWIDGET_H
