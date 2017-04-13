#include "functionswidget.h"
#include "ui_functionswidget.h"

#include "mainwindow.h"
#include "helpers.h"
#include "dialogs/commentsdialog.h"
#include "dialogs/renamedialog.h"
#include "dialogs/xrefsdialog.h"

#include <QTreeWidget>
#include <QMenu>
#include <QDebug>
#include <QString>


FunctionsWidget::FunctionsWidget(MainWindow *main, QWidget *parent) :
    DockWidget(parent),
    ui(new Ui::FunctionsWidget),
    main(main)
{
    ui->setupUi(this);

    ui->functionsTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    //ui->functionsTreeWidget->setFont(QFont("Monospace", 8));
    // Set Functions context menu
    connect(ui->functionsTreeWidget, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showFunctionsContextMenu(const QPoint &)));
    connect(ui->nestedFunctionsTree, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showFunctionsContextMenu(const QPoint &)));

    ui->functionsTreeWidget->hideColumn(0);

    // Hide the tabs
    QTabBar *tabs = ui->tabWidget->tabBar();
    tabs->setVisible(false);

    // Use a custom context menu on the dock title bar
    //this->title_bar = this->titleBarWidget();
    ui->actionHorizontal->setChecked(true);
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showTitleContextMenu(const QPoint &)));
}

FunctionsWidget::~FunctionsWidget()
{
    delete ui;
}

void FunctionsWidget::setup()
{
    setScrollMode();

    fillFunctions();
}

void FunctionsWidget::refresh()
{
    setup();
}

void FunctionsWidget::fillFunctions()
{
    ui->functionsTreeWidget->clear();
    ui->nestedFunctionsTree->clear();
    for (auto i : this->main->core->getList("anal", "functions"))
    {
        QStringList a = i.split(",");
        // off,sz,unk,name
        // "0x0804ada3,1,13,,fcn.0804ada3"
        // "0x0804ad4a,6,,1,,fcn.0804ad4a"
        if (a.length() == 5)
        {
            // Add list function
            qhelpers::appendRow(ui->functionsTreeWidget, a[0], a[1], a[4]);
            // Add nested function
            QTreeWidgetItem *item = new QTreeWidgetItem(ui->nestedFunctionsTree);
            item->setText(0, a[4]);
            QTreeWidgetItem *size_it = new QTreeWidgetItem();
            size_it->setText(0, "Offset: " + a[0]);
            item->addChild(size_it);
            QTreeWidgetItem *off_it = new QTreeWidgetItem();
            off_it->setText(0, "Size: " + a[1]);
            item->addChild(off_it);
            ui->nestedFunctionsTree->addTopLevelItem(item);
        }
        else if (a.length() == 6)
        {
            // Add list function
            qhelpers::appendRow(ui->functionsTreeWidget, a[0], a[1], a[5]);
            // Add nested function
            QTreeWidgetItem *item = new QTreeWidgetItem(ui->nestedFunctionsTree);
            item->setText(0, a[5]);
            QTreeWidgetItem *size_it = new QTreeWidgetItem();
            size_it->setText(0, "Offset: " + a[0]);
            item->addChild(size_it);
            QTreeWidgetItem *off_it = new QTreeWidgetItem();
            off_it->setText(0, "Size: " + a[1]);
            item->addChild(off_it);
            ui->nestedFunctionsTree->addTopLevelItem(item);
        }
        else
        {
            qDebug() << "fillFunctions()" << a;
        }
    }
    ui->functionsTreeWidget->sortByColumn(3, Qt::AscendingOrder);
    ui->nestedFunctionsTree->sortByColumn(0, Qt::AscendingOrder);
    qhelpers::adjustColumns(ui->functionsTreeWidget);

    this->addTooltips();
}

void FunctionsWidget::on_functionsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    QNOTUSED(column);

    QString offset = item->text(1);
    QString name = item->text(3);
    this->main->seek(offset, name);
    this->main->raiseMemoryDock();
}

void FunctionsWidget::showFunctionsContextMenu(const QPoint &pt)
{
    // Set functions popup menu
    QMenu *menu = new QMenu(ui->functionsTreeWidget);
    menu->clear();
    menu->addAction(ui->actionDisasAdd_comment);
    menu->addAction(ui->actionFunctionsRename);
    menu->addAction(ui->actionFunctionsUndefine);
    menu->addSeparator();
    menu->addAction(ui->action_References);

    if (ui->tabWidget->currentIndex() == 0)
    {
        ui->functionsTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
        menu->exec(ui->functionsTreeWidget->mapToGlobal(pt));
    }
    else
    {
        ui->nestedFunctionsTree->setContextMenuPolicy(Qt::CustomContextMenu);
        menu->exec(ui->nestedFunctionsTree->mapToGlobal(pt));
    }
    delete menu;
}

void FunctionsWidget::refreshTree()
{
    ui->functionsTreeWidget->clear();
    ui->nestedFunctionsTree->clear();
    for (auto i : this->main->core->getList("anal", "functions"))
    {
        QStringList a = i.split(",");
        // off,sz,unk,name
        // "0x0804ada3,1,13,,fcn.0804ada3"
        // "0x0804ad4a,6,,1,,fcn.0804ad4a"
        if (a.length() == 5)
        {
            // Add list function
            qhelpers::appendRow(ui->functionsTreeWidget, a[0], a[1], a[4]);
            // Add nested function
            QTreeWidgetItem *item = new QTreeWidgetItem(ui->nestedFunctionsTree);
            item->setText(0, a[4]);
            QTreeWidgetItem *size_it = new QTreeWidgetItem();
            size_it->setText(0, "Offset: " + a[0]);
            item->addChild(size_it);
            QTreeWidgetItem *off_it = new QTreeWidgetItem();
            off_it->setText(0, "Size: " + a[1]);
            item->addChild(off_it);
            ui->nestedFunctionsTree->addTopLevelItem(item);
        }
        else if (a.length() == 6)
        {
            // Add list function
            qhelpers::appendRow(ui->functionsTreeWidget, a[0], a[1], a[5]);
            // Add nested function
            QTreeWidgetItem *item = new QTreeWidgetItem(ui->nestedFunctionsTree);
            item->setText(0, a[5]);
            QTreeWidgetItem *size_it = new QTreeWidgetItem();
            size_it->setText(0, "Offset: " + a[0]);
            item->addChild(size_it);
            QTreeWidgetItem *off_it = new QTreeWidgetItem();
            off_it->setText(0, "Size: " + a[1]);
            item->addChild(off_it);
            ui->nestedFunctionsTree->addTopLevelItem(item);
        }
        else
        {
            qDebug() << "fillFunctions()" << a;
        }
    }
    ui->functionsTreeWidget->sortByColumn(3, Qt::AscendingOrder);
    ui->nestedFunctionsTree->sortByColumn(0, Qt::AscendingOrder);
    qhelpers::adjustColumns(ui->functionsTreeWidget);

    this->addTooltips();
}

void FunctionsWidget::on_actionDisasAdd_comment_triggered()
{
    QString fcn_name = "";
    // Create dialog
    CommentsDialog *c = new CommentsDialog(this);
    // Get selected item in functions tree widget
    if (ui->tabWidget->currentIndex() == 0)
    {
        QList<QTreeWidgetItem *> selected_rows = ui->functionsTreeWidget->selectedItems();
        // Get selected function name
        fcn_name = selected_rows.first()->text(3);
    }
    else
    {
        QList<QTreeWidgetItem *> selected_rows = ui->nestedFunctionsTree->selectedItems();
        // Get selected function name
        fcn_name = selected_rows.first()->text(0);
    }
    if (c->exec())
    {
        // Get new function name
        QString comment = c->getComment();
        this->main->add_debug_output("Comment: " + comment + " at: " + fcn_name);
        // Rename function in r2 core
        this->main->core->setComment(fcn_name, comment);
        // Seek to new renamed function
        this->main->seek(fcn_name);
        // TODO: Refresh functions tree widget
    }
    this->main->refreshComments();
}

void FunctionsWidget::addTooltips()
{

    // Add comments to list functions
    QList<QTreeWidgetItem *> clist = ui->functionsTreeWidget->findItems("*", Qt::MatchWildcard, 3);
    foreach (QTreeWidgetItem *item, clist)
    {
        QString name = item->text(3);
        QList<QString> info = this->main->core->cmd("afi @ " + name).split("\n");
        if (info.length() > 2)
        {
            QString size = info[4].split(" ")[1];
            QString complex = info[8].split(" ")[1];
            QString bb = info[11].split(" ")[1];
            item->setToolTip(3, "Summary:\n\n    Size: " + size +
                             "\n    Cyclomatic complexity: " + complex +
                             "\n    Basic blocks: " + bb +
                             "\n\nDisasm preview:\n\n" + this->main->core->cmd("pdi 10 @ " + name) +
                             "\nStrings:\n\n" + this->main->core->cmd("pdsf @ " + name));
            //"\nStrings:\n\n" + this->main->core->cmd("pds @ " + name + "!$F"));
        }
    }

    // Add comments to nested functions
    QList<QTreeWidgetItem *> nlist = ui->nestedFunctionsTree->findItems("*", Qt::MatchWildcard, 0);
    foreach (QTreeWidgetItem *item, nlist)
    {
        QString name = item->text(0);
        QList<QString> info = this->main->core->cmd("afi @ " + name).split("\n");
        if (info.length() > 2)
        {
            QString size = info[4].split(" ")[1];
            QString complex = info[8].split(" ")[1];
            QString bb = info[11].split(" ")[1];
            item->setToolTip(0, "Summary:\n\n    Size: " + size +
                             "\n    Cyclomatic complexity: " + complex +
                             "\n    Basic blocks: " + bb +
                             "\n\nDisasm preview:\n\n" + this->main->core->cmd("pdi 10 @ " + name) +
                             "\nStrings:\n\n" + this->main->core->cmd("pdsf @ " + name));
            //"\nStrings:\n\n" + this->main->core->cmd("pds @ " + name + "!$F"));
        }
    }
}

void FunctionsWidget::on_actionFunctionsRename_triggered()
{
    // Create dialog
    RenameDialog *r = new RenameDialog(this);
    // Get selected item in functions tree widget
    QList<QTreeWidgetItem *> selected_rows = ui->functionsTreeWidget->selectedItems();
    // Get selected function name
    QString old_name = selected_rows.first()->text(3);
    // Set function name in dialog
    r->setFunctionName(old_name);
    // If user accepted
    if (r->exec())
    {
        // Get new function name
        QString new_name = r->getFunctionName();
        // Rename function in r2 core
        this->main->core->renameFunction(old_name, new_name);
        // Change name in functions tree widget
        selected_rows.first()->setText(3, new_name);
        // Scroll to show the new name in functions tree widget
        /*
         * QAbstractItemView::EnsureVisible
         * QAbstractItemView::PositionAtTop
         * QAbstractItemView::PositionAtBottom
         * QAbstractItemView::PositionAtCenter
         */
        ui->functionsTreeWidget->scrollToItem(selected_rows.first(), QAbstractItemView::PositionAtTop);
        // Seek to new renamed function
        this->main->seek(new_name);
    }
}

void FunctionsWidget::on_action_References_triggered()
{
    QList<QTreeWidgetItem *> selected_rows = ui->functionsTreeWidget->selectedItems();
    // Get selected function address
    QString address = selected_rows.first()->text(1);

    //this->main->add_debug_output("Addr: " + address);

    // Get function for clicked offset
    RAnalFunction *fcn = this->main->core->functionAt(address.toLongLong(0, 16));

    XrefsDialog *x = new XrefsDialog(this->main, this);
    x->setWindowTitle("X-Refs for function " + QString::fromUtf8(fcn->name));

    // Get Refs and Xrefs
    QList<QStringList> ret_refs;
    QList<QStringList> ret_xrefs;

    // refs = calls q hace esa funcion
    QList<QString> refs = this->main->core->getFunctionRefs(fcn->addr, 'C');
    if (refs.size() > 0)
    {
        for (int i = 0; i < refs.size(); ++i)
        {
            //this->main->add_debug_output(refs.at(i));
            QStringList retlist = refs.at(i).split(",");
            QStringList temp;
            QString addr = retlist.at(2);
            temp << addr;
            QString op = this->main->core->cmd("pi 1 @ " + addr);
            temp << op.simplified();
            ret_refs << temp;
        }
    }

    // xrefs = calls a esa funcion
    //qDebug() << this->main->core->getFunctionXrefs(offset.toLong(&ok, 16));
    QList<QString> xrefs = this->main->core->getFunctionXrefs(fcn->addr);
    if (xrefs.size() > 0)
    {
        for (int i = 0; i < xrefs.size(); ++i)
        {
            //this->main->add_debug_output(xrefs.at(i));
            QStringList retlist = xrefs.at(i).split(",");
            QStringList temp;
            QString addr = retlist.at(1);
            temp << addr;
            QString op = this->main->core->cmd("pi 1 @ " + addr);
            temp << op.simplified();
            ret_xrefs << temp;
        }
    }
    x->fillRefs(ret_refs, ret_xrefs);
    x->exec();
}

void FunctionsWidget::showTitleContextMenu(const QPoint &pt)
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

void FunctionsWidget::on_actionHorizontal_triggered()
{
    ui->tabWidget->setCurrentIndex(0);
}

void FunctionsWidget::on_actionVertical_triggered()
{
    ui->tabWidget->setCurrentIndex(1);
}

void FunctionsWidget::on_nestedFunctionsTree_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    QNOTUSED(column);

    //QString offset = item->text(1);
    QString name = item->text(0);
    QString offset = item->child(0)->text(0).split(":")[1];
    this->main->seek(offset, name);
    this->main->raiseMemoryDock();
}

void FunctionsWidget::resizeEvent(QResizeEvent *event)
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

void FunctionsWidget::setScrollMode()
{
    qhelpers::setVerticalScrollMode(ui->functionsTreeWidget);
}
