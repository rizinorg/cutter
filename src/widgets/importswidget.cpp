#include "importswidget.h"
#include "ui_importswidget.h"

#include <QStyledItemDelegate>
#include "widgets/banned.h"
#include "mainwindow.h"

void CMyDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem itemOption(option);
    initStyleOption(&itemOption, index);

    itemOption.rect.adjust(10, 0, 0, 0);  // Make the item rectangle 10 pixels smaller from the left side.

    // Draw your item content.
    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &itemOption, painter, nullptr);

    // And now you can draw a bottom border.
    //painter->setPen(Qt::cyan);
    QPen pen = painter->pen();
    pen.setColor(Qt::white);
    pen.setWidth(1);
    painter->setPen(pen);
    painter->drawLine(itemOption.rect.bottomLeft(), itemOption.rect.bottomRight());
}

/*
 * Imports Widget
 */

ImportsWidget::ImportsWidget(MainWindow *main, QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::ImportsWidget)
{
    ui->setupUi(this);

    // Radare core found in:
    this->main = main;
    this->importsTreeWidget = ui->importsTreeWidget;

    // Delegate
    //CMyDelegate* delegate = new CMyDelegate(ui->importsTreeWidget);
    //ui->importsTreeWidget->setItemDelegate(delegate);
}

void ImportsWidget::fillImports()
{
    this->importsTreeWidget->clear();
    for (auto i : this->main->core->getList("bin", "imports"))
    {
        QStringList a = i.split(",");
        // ord,plt,name
        if (a.length() == 6)
            this->main->appendRow(this->importsTreeWidget, a[1], a[3], "", a[4]);
    }
    highlightUnsafe();
    this->main->adjustColumns(this->importsTreeWidget);
}

void ImportsWidget::highlightUnsafe()
{
    Banned *ban = new Banned();
    QList<QTreeWidgetItem *> clist = this->importsTreeWidget->findItems(ban->banned, Qt::MatchRegExp, 4);
    foreach (QTreeWidgetItem *item, clist)
    {
        item->setText(3, "Unsafe");
        //item->setBackgroundColor(4, QColor(255, 129, 123));
        //item->setForeground(4, Qt::white);
        item->setForeground(4, QColor(255, 129, 123));
    }
    //ui->importsTreeWidget->setStyleSheet("QTreeWidget::item { padding-left:10px; padding-top: 1px; padding-bottom: 1px; border-left: 10px; }");
}

void ImportsWidget::adjustColumns(QTreeWidget *tw)
{
    int count = tw->columnCount();
    for (int i = 0; i != count; ++i)
    {
        ui->importsTreeWidget->resizeColumnToContents(i);
        int width = ui->importsTreeWidget->columnWidth(i);
        ui->importsTreeWidget->setColumnWidth(i, width + 10);
    }
}

ImportsWidget::~ImportsWidget()
{
    delete ui;
}
