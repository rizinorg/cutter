#ifndef IMPORTSWIDGET_H
#define IMPORTSWIDGET_H

#include <QStyledItemDelegate>
#include <QDockWidget>
#include <QTreeWidget>

class MainWindow;

namespace Ui
{
    class ImportsWidget;
}

class ImportsWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit ImportsWidget(MainWindow *main, QWidget *parent = 0);
    ~ImportsWidget();

    QTreeWidget    *importsTreeWidget;
    void fillImports();
    void highlightUnsafe();

private:
    Ui::ImportsWidget *ui;

    MainWindow      *main;
    void adjustColumns(QTreeWidget *tw);
};

class CMyDelegate : public QStyledItemDelegate
{
public:
    explicit CMyDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif // IMPORTSWIDGET_H
