#include <QTreeWidget>
#include <QMenu>
#include <QResizeEvent>

#include "CommentsWidget.h"
#include "ui_CommentsWidget.h"
#include "MainWindow.h"
#include "utils/Helpers.h"

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

    connect(CutterCore::getInstance(), SIGNAL(commentsChanged()), this, SLOT(refreshTree()));

    // Hide the buttons frame
    ui->frame->hide();
}

CommentsWidget::~CommentsWidget() {}

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
    Q_UNUSED(column);

    // Get offset and name of item double clicked
    CommentDescription comment = item->data(0, Qt::UserRole).value<CommentDescription>();
    this->main->addDebugOutput(RAddressString(comment.offset) + ": " + comment.name);
    CutterCore::getInstance()->seek(comment.offset);
    //CutterCore::getInstance()->seek(comment.offset, comment.name, true);
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
    if (main->responsive && isVisible())
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
    ui->nestedCmtsTreeWidget->clear();
    QList<CommentDescription> comments = CutterCore::getInstance()->getAllComments("CCu");

    for (CommentDescription comment : comments)
    {
        //this->main->add_debug_output(RAddressString(comment.offset));
        QString fcn_name = CutterCore::getInstance()->cmdFunctionAt(comment.offset);
        QTreeWidgetItem *item = qhelpers::appendRow(ui->commentsTreeWidget, RAddressString(comment.offset), fcn_name, comment.name);
        item->setData(0, Qt::UserRole, QVariant::fromValue(comment));
    }
    qhelpers::adjustColumns(ui->commentsTreeWidget);

    // Add nested comments
    ui->nestedCmtsTreeWidget->clear();
    QMap<QString, QList<QList<QString>>> cmts = CutterCore::getInstance()->getNestedComments();
    for (auto cmt : cmts.keys())
    {
        QTreeWidgetItem *item = new QTreeWidgetItem(ui->nestedCmtsTreeWidget);
        item->setText(0, cmt);
        QList<QList<QString>> meow = cmts.value(cmt);
        for (int i = 0; i < meow.size(); ++i)
        {
            QList<QString> tmp = meow.at(i);
            QTreeWidgetItem *it = new QTreeWidgetItem();
            it->setText(0, tmp[1]);
            it->setText(1, tmp[0].remove('"'));
            item->addChild(it);
        }
        ui->nestedCmtsTreeWidget->addTopLevelItem(item);
    }
    qhelpers::adjustColumns(ui->nestedCmtsTreeWidget);
}
