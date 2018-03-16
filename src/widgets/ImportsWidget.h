#ifndef IMPORTSWIDGET_H
#define IMPORTSWIDGET_H

#include <memory>

#include <QStyledItemDelegate>
#include <QTreeWidgetItem>

#include "CutterDockWidget.h"

class MainWindow;
class QTreeWidget;

namespace Ui
{
    class ImportsWidget;
}

class ImportsWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit ImportsWidget(MainWindow *main, QAction *action);
    ~ImportsWidget();

private slots:
    void on_importsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);

    void fillImports();

private:
    std::unique_ptr<Ui::ImportsWidget> ui;

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
