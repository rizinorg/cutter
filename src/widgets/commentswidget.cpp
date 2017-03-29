#include "commentswidget.h"
#include "ui_commentswidget.h"

#include "mainwindow.h"

CommentsWidget::CommentsWidget(MainWindow *main, QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::CommentsWidget)
{
    ui->setupUi(this);

    // Radare core found in:
    this->main = main;
    this->commentsTreeWidget = ui->commentsTreeWidget;
    this->nestedCommentsTreeWidget = ui->nestedCmtsTreeWidget;

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

    // Resize eventfilter
    this->installEventFilter(this);
    ui->commentsTreeWidget->viewport()->installEventFilter(this);
}

CommentsWidget::~CommentsWidget()
{
    delete ui;
}

void CommentsWidget::on_commentsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    // Get offset and name of item double clicked
    // TODO: use this info to change disasm contents
    QString offset = item->text(1);
    QString name = item->text(2);
    this->main->add_debug_output(offset + ": " + name);
    this->main->seek (offset, name);
}

void CommentsWidget::refreshTree() {
    this->commentsTreeWidget->clear();
    QList<QList<QString>> comments = this->main->core->getComments();
    for (QList<QString> comment: comments) {
        this->main->add_debug_output(comment[1]);
        QString fcn_name = this->main->core->cmdFunctionAt(comment[1]);
        this->main->appendRow(this->commentsTreeWidget, comment[1], fcn_name, comment[0].remove('"'));
    }
    this->main->adjustColumns(this->commentsTreeWidget);

    // Add nested comments
    this->nestedCommentsTreeWidget->clear();
    QMap<QString, QList<QList<QString>>> cmts = this->main->core->getNestedComments();
    for(auto cmt : cmts.keys()) {
        QTreeWidgetItem *item = new QTreeWidgetItem(this->nestedCommentsTreeWidget);
        item->setText(0, cmt);
        QList<QList<QString>> meow = cmts.value(cmt);
        for (int i = 0; i < meow.size(); ++i) {
            QList<QString> tmp = meow.at(i);
            QTreeWidgetItem *it = new QTreeWidgetItem();
            it->setText(0, tmp[1]);
            it->setText(1, tmp[0].remove('"'));
            item->addChild(it);
        }
        this->nestedCommentsTreeWidget->addTopLevelItem(item);
    }
    this->main->adjustColumns(this->nestedCommentsTreeWidget);
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

    if (ui->tabWidget->currentIndex() == 0) {
        ui->actionHorizontal->setChecked(true);
        ui->actionVertical->setChecked(false);
    } else {
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

bool CommentsWidget::eventFilter(QObject *obj, QEvent *event) {
    if (this->main->responsive) {
        if (event->type() == QEvent::Resize && obj == this && this->isVisible()) {
            QResizeEvent *resizeEvent = static_cast<QResizeEvent*>(event);
            //qDebug("Dock Resized (New Size) - Width: %d Height: %d",
            //       resizeEvent->size().width(),
            //       resizeEvent->size().height());
            if (resizeEvent->size().width() >= resizeEvent->size().height()) {
                // Set horizontal view (list)
                this->on_actionHorizontal_triggered();
            } else {
                // Set vertical view (Tree)
                this->on_actionVertical_triggered();
            }
        }
    }
}
