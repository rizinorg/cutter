#include "commentswidget.h"
#include "ui_commentswidget.h"

#include "mainwindow.h"
#include "helpers.h"

#include <QTreeWidget>
#include <QMenu>
#include <QResizeEvent>


CommentsWidget::CommentsWidget(MainWindow *main, QWidget *parent) :
    DockWidget(parent),
    ui(new Ui::CommentsWidget),
    main(main)
{
    ui->setupUi(this);

    ui->commentsTreeWidget->hideColumn(0);

    QTabBar *tabs = ui->tabWidget->tabBar();
    tabs->setVisible(false);

    // Use a custom context menu on the dock title bar
    //this->title_bar = this->titleBarWidget();
    ui->actionHorizontal->setChecked(true);
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showTitleContextMenu(const QPoint &)));

    // Hide the buttons frame
    ui->frame->hide();
}

CommentsWidget::~CommentsWidget()
{
    delete ui;
}

void CommentsWidget::setup()
{
    refreshTree();
}

void CommentsWidget::refresh()
{
    refreshTree();
}

void CommentsWidget::on_commentsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    QNOTUSED(column);

    // Get offset and name of item double clicked
    // TODO: use this info to change disasm contents
    QString offset = item->text(1);
    QString name = item->text(2);
    this->main->add_debug_output(offset + ": " + name);
    this->main->seek(offset, name);
}

void CommentsWidget::on_toolButton_clicked()
{
    ui->tabWidget->setCurrentIndex(0);
}

void CommentsWidget::on_toolButton_2_clicked()
{
    ui->tabWidget->setCurrentIndex(1);
}

void CommentsWidget::showTitleContextMenu(const QPoint &pt)
{
    // Set functions popup menu
    QMenu *menu = new QMenu(this);
    menu->clear();
    menu->addAction(ui->actionHorizontal);
    menu->addAction(ui->actionVertical);

    if (ui->tabWidget->currentIndex() == 0)
    {
        ui->actionHorizontal->setChecked(true);
        ui->actionVertical->setChecked(false);
    }
    else
    {
        ui->actionVertical->setChecked(true);
        ui->actionHorizontal->setChecked(false);
    }

    this->setContextMenuPolicy(Qt::CustomContextMenu);

    menu->exec(this->mapToGlobal(pt));
    delete menu;
}

void CommentsWidget::on_actionHorizontal_triggered()
{
    ui->tabWidget->setCurrentIndex(0);
}

void CommentsWidget::on_actionVertical_triggered()
{
    ui->tabWidget->setCurrentIndex(1);
}

void CommentsWidget::resizeEvent(QResizeEvent *event)
{
    if(main->responsive && isVisible())
    {
        if (event->size().width() >= event->size().height())
        {
            // Set horizontal view (list)
            on_actionHorizontal_triggered();
        }
        else
        {
            // Set vertical view (Tree)
            on_actionVertical_triggered();
        }
    }
    QDockWidget::resizeEvent(event);
}

void CommentsWidget::refreshTree()
{
    ui->commentsTreeWidget->clear();
    const QList<QList<QString>> &comments = main->core->getComments();
    for (const QList<QString> &comment : comments)
    {
        //this->main->add_debug_output(comment[1]);
        QString fcn_name = this->main->core->cmdFunctionAt(comment[1]);
        qhelpers::appendRow(ui->commentsTreeWidget, comment[1], fcn_name, QString(comment[0]).remove('"'));
    }
    qhelpers::adjustColumns(ui->commentsTreeWidget);

    // Add nested comments
    ui->nestedCmtsTreeWidget->clear();
    const QMap<QString, QList<QList<QString>>> &cmts = main->core->getNestedComments();
    for (auto cmt : cmts.keys())
    {
        QTreeWidgetItem *item = new QTreeWidgetItem(ui->nestedCmtsTreeWidget);
        item->setText(0, cmt);
        const QList<QList<QString>> &meow = cmts.value(cmt);
        for (int i = 0; i < meow.size(); ++i)
        {
            const QList<QString> &tmp = meow.at(i);
            QTreeWidgetItem *it = new QTreeWidgetItem();
            it->setText(0, tmp[1]);
            it->setText(1, QString(tmp[0]).remove('"'));
            item->addChild(it);
        }
        ui->nestedCmtsTreeWidget->addTopLevelItem(item);
    }
    qhelpers::adjustColumns(ui->nestedCmtsTreeWidget);
}
