#include <QTreeWidget>
#include <QMenu>
#include <QResizeEvent>

#include "CommentsWidget.h"
#include "ui_CommentsWidget.h"
#include "MainWindow.h"
#include "utils/Helpers.h"

CommentsWidget::CommentsWidget(MainWindow *main, QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::CommentsWidget),
    main(main)
{
    ui->setupUi(this);

    ui->commentsTreeWidget->sortByColumn(2, Qt::AscendingOrder);

    QTabBar *tabs = ui->tabWidget->tabBar();
    tabs->setVisible(false);

    // Use a custom context menu on the dock title bar
    //this->title_bar = this->titleBarWidget();
    ui->actionHorizontal->setChecked(true);
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showTitleContextMenu(const QPoint &)));

    connect(Core(), SIGNAL(commentsChanged()), this, SLOT(refreshTree()));
    connect(Core(), SIGNAL(refreshAll()), this, SLOT(refreshTree()));

    // Hide the buttons frame
    ui->frame->hide();
}

CommentsWidget::~CommentsWidget() {}

void CommentsWidget::on_commentsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int)
{
    // Get offset and name of item double clicked
    CommentDescription comment = item->data(0, Qt::UserRole).value<CommentDescription>();
    CutterCore::getInstance()->seek(comment.offset);
}

void CommentsWidget::on_nestedCmtsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int)
{
    // don't react on top-level items
    if (item->parent() == nullptr)
    {
        return;
    }

    // Get offset and name of item double clicked
    CommentDescription comment = item->data(0, Qt::UserRole).value<CommentDescription>();
    CutterCore::getInstance()->seek(comment.offset);

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

/*
 *
QMap<QString, QList<QList<QString>>> CutterCore::getNestedComments()
{
    QMap<QString, QList<QList<QString>>> ret;
    QString comments = cmd("CC~CCu");

    for (QString line : comments.split("\n"))
    {
        QStringList fields = line.split("CCu");
        if (fields.length() == 2)
        {
            QList<QString> tmp = QList<QString>();
            tmp << fields[1].split("\"")[1].trimmed();
            tmp << fields[0].trimmed();
            QString fcn_name = this->cmdFunctionAt(fields[0].trimmed());
            ret[fcn_name].append(tmp);
        }
    }
    return ret;
}
 */

void CommentsWidget::refreshTree()
{
    ui->nestedCmtsTreeWidget->clear();
    QList<CommentDescription> comments = CutterCore::getInstance()->getAllComments("CCu");
    QMap<QString, QList<CommentDescription>> nestedComments;

    for (CommentDescription comment : comments)
    {
        QString fcn_name = CutterCore::getInstance()->cmdFunctionAt(comment.offset);
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, RAddressString(comment.offset));
        item->setText(1, fcn_name);
        item->setText(2, comment.name);
        item->setData(0, Qt::UserRole, QVariant::fromValue(comment));
        ui->commentsTreeWidget->addTopLevelItem(item);

        nestedComments[fcn_name].append(comment);
    }
    qhelpers::adjustColumns(ui->commentsTreeWidget, 0);

    // Add nested comments
    ui->nestedCmtsTreeWidget->clear();
    for (auto functionName : nestedComments.keys())
    {
        QTreeWidgetItem *item = new QTreeWidgetItem(ui->nestedCmtsTreeWidget);
        item->setText(0, functionName);
        for (CommentDescription comment : nestedComments.value(functionName))
        {
            QTreeWidgetItem *it = new QTreeWidgetItem();
            it->setText(0, RAddressString(comment.offset));
            it->setText(1, comment.name);
            it->setData(0, Qt::UserRole, QVariant::fromValue(comment));
            item->addChild(it);
        }
        ui->nestedCmtsTreeWidget->addTopLevelItem(item);
    }
    qhelpers::adjustColumns(ui->nestedCmtsTreeWidget, 0);
}
