#ifndef IMPORTSWIDGET_H
#define IMPORTSWIDGET_H

#include "dockwidget.h"
#include <QStyledItemDelegate>
#include <QTreeWidgetItem>

class MainWindow;
class QTreeWidget;

namespace Ui
{
    class ImportsWidget;
}

class ImportsWidget : public DockWidget
{
    Q_OBJECT

public:
    explicit ImportsWidget(MainWindow *main, QWidget *parent = 0);
    ~ImportsWidget();

    void setup() override;

    void refresh() override;

private slots:
    void on_importsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);

private:
    Ui::ImportsWidget *ui;
    MainWindow      *main;

    void fillImports();
    void highlightUnsafe();
    void setScrollMode();
};

class CMyDelegate : public QStyledItemDelegate
{
public:
    explicit CMyDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif // IMPORTSWIDGET_H
