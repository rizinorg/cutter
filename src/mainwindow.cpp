#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "createnewdialog.h"
#include "dialogs/commentsdialog.h"
#include "dialogs/aboutdialog.h"
#include "dialogs/renamedialog.h"

#include <qfont.h>
#include <qsettings.h>
#include <qlabel.h>
#include <qfile.h>
#include <qmessagebox.h>
#include <qfontdialog.h>
#include <qcompleter.h>
#include <qstringlistmodel.h>
#include <QScrollBar>
#include <QDir>
#include <QDebug>
#include <QDesktopServices>
#include <QFileDialog>
#include <QPropertyAnimation>
#include <QStyledItemDelegate>
#include <QProcess>

#include <QtGlobal>
#include <QToolTip>
#include <QShortcut>
#include <QTextCursor>
#include <QMessageBox>
#include <QStyleFactory>

// graphics
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include <QGraphicsView>

static void adjustColumns(QTreeWidget *tw) {
    int count = tw->columnCount();
    for (int i = 0; i != count; ++i) {
        tw->resizeColumnToContents(i);
    }
}

static void appendRow(QTreeWidget *tw, const QString &str, const QString &str2=NULL,
                      const QString &str3=NULL, const QString &str4=NULL, const QString &str5=NULL) {
    QTreeWidgetItem *tempItem = new QTreeWidgetItem();
    // Fill dummy hidden column
    tempItem->setText(0,"0");
    tempItem->setText(1,str);
    if (str2!=NULL)
        tempItem->setText(2, str2);
    if (str3!=NULL)
        tempItem->setText(3, str3);
    if (str4!=NULL)
        tempItem->setText(4, str4);
    if (str5!=NULL)
        tempItem->setText(5, str5);
    tw->insertTopLevelItem(0, tempItem);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    core(new QRCore()),
    ui(new Ui::MainWindow),
    webserverThread(core, this)
{
    ui->setupUi(this);

    doLock = false;

    // Add custom font
    QFontDatabase::addApplicationFont(":/new/prefix1/fonts/Anonymous Pro.ttf");

    /*
    * Toolbar
    */
    // Hide central tab widget tabs
    QTabBar *centralbar = ui->centralTabWidget->tabBar();
    centralbar->setVisible(false);
    // Adjust console lineedit
    ui->consoleInputLineEdit->setTextMargins(10, 0, 0, 0);
    /*
    ui->consoleOutputTextEdit->setFont(QFont("Monospace", 8));
    ui->consoleOutputTextEdit->setStyleSheet("background-color:black;color:gray;");
    ui->consoleInputLineEdit->setStyleSheet("background-color:black;color:gray;");
    */

    // Adjust text margins of consoleOutputTextEdit
    QTextDocument *console_docu = ui->consoleOutputTextEdit->document();
    console_docu->setDocumentMargin(10);

    // Sepparator between back/forward and undo/redo buttons
    QWidget* spacer4 = new QWidget();
    spacer4->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    spacer4->setMinimumSize(10, 10);
    ui->mainToolBar->insertWidget(ui->actionForward, spacer4);

    // Popup menu on theme toolbar button
    QToolButton *backButton = new QToolButton(this);
    backButton->setIcon(QIcon(":/new/prefix1/img/icons/arrow_left.png"));
    //backButton->setPopupMode(QToolButton::DelayedPopup);
    ui->mainToolBar->insertWidget(ui->actionForward, backButton);
    //connect(backButton, SIGNAL(clicked()), this, SLOT(on_backButton_clicked()));

    // Sepparator between undo/redo and goto lineEdit
    QWidget* spacer3 = new QWidget();
    spacer3->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    spacer3->setMinimumSize(20, 20);
    spacer3->setMaximumWidth(300);
    ui->mainToolBar->insertWidget(ui->actionShow_Hide_mainsidebar, spacer3);

    // Omnibar LineEdit
    this->omnibar = new Omnibar(this);
    ui->mainToolBar->insertWidget(ui->actionShow_Hide_mainsidebar, this->omnibar);

    // Add special sepparators to the toolbar that expand to separate groups of elements
    QWidget* spacer2 = new QWidget();
    spacer2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    spacer2->setMinimumSize(10, 10);
    spacer2->setMaximumWidth(300);
    ui->mainToolBar->insertWidget(ui->actionShow_Hide_mainsidebar, spacer2);

    // Sepparator between back/forward and undo/redo buttons
    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    spacer->setMinimumSize(20, 20);
    ui->mainToolBar->addWidget(spacer);

    // codeGraphics tool bar
    this->graphicsBar = new GraphicsBar(this);
    this->graphicsBar->setMovable(false);
    addToolBarBreak(Qt::TopToolBarArea);
    addToolBar(graphicsBar);

    // Fix output panel font
    QHelpers *help = new QHelpers();
    help->normalizeFont(ui->consoleOutputTextEdit);

    /*
     * Dock Widgets
     */

    // Add Memory DockWidget
    this->memoryDock = new MemoryWidget(this);
    this->dockList << this->memoryDock;
    // To use in the future when we handle more than one memory views
    // this->memoryDock->setAttribute(Qt::WA_DeleteOnClose);
    // this->add_debug_output( QString::number(this->dockList.length()) );

    // Add Sections dock panel
    this->sectionsWidget = new SectionsWidget(this);
    this->sectionsDock = new QDockWidget("Sections");
    this->sectionsDock->setObjectName("sectionsDock");
    this->sectionsDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    this->sectionsDock->setWidget(this->sectionsWidget);
    this->sectionsWidget->setContentsMargins(0,0,0,5);
    this->sectionsDock->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    this->sectionsDock->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this->sectionsDock, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showSectionsContextMenu(const QPoint &)));

    // Add functions DockWidget
    this->functionsDock = new FunctionsWidget(this);

    // Add imports DockWidget
    this->importsDock = new ImportsWidget(this);

    // Add symbols DockWidget
    this->symbolsDock = new SymbolsWidget(this);

    // Add relocs DockWidget
    this->relocsDock = new RelocsWidget(this);

    // Add comments DockWidget
    this->commentsDock = new CommentsWidget(this);

    // Add strings DockWidget
    this->stringsDock = new StringsWidget(this);

    // Add flags DockWidget
    this->flagsDock = new FlagsWidget(this);

    // Add Notepad Dock panel
    this->notepadDock = new Notepad(this);

    //Add Dashboard Dock panel
    this->dashboardDock = new Dashboard(this);

    // Set up dock widgets default layout
    restoreDocks();
    hideAllDocks();
    showDefaultDocks();

    // Restore saved settings
    this->readSettings();
    // TODO: Allow the user to select this option visually in the GUI settings
    // Adjust the DockWidget areas
    setCorner( Qt::TopLeftCorner, Qt::LeftDockWidgetArea );
    //setCorner( Qt::TopRightCorner, Qt::RightDockWidgetArea );
    setCorner( Qt::BottomLeftCorner, Qt::LeftDockWidgetArea );
    //setCorner( Qt::BottomRightCorner, Qt::RightDockWidgetArea );

    this->flagsDock->flagsTreeWidget->clear();

    // Set omnibar completer for flags
    this->omnibar->setupCompleter();

    // Set console output context menu
    ui->consoleOutputTextEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->consoleOutputTextEdit, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showConsoleContextMenu(const QPoint &)));

    // Hide dummy columns so we can reorder the rest
    hideDummyColumns();

    // Setup and hide sidebar by default
    this->sideBar = new SideBar(this);
    this->sidebar_action = ui->sideToolBar->addWidget(this->sideBar);
    ui->sideToolBar->hide();

    // Show dashboard by default
    this->dashboardDock->raise();

    //qDebug() << "FOLDER: " << QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    /*
     *  Some global shortcuts
     */
    // Period goes to command entry
    QShortcut* cmd_shortcut = new QShortcut(QKeySequence(Qt::Key_Period), this);
    connect(cmd_shortcut, SIGNAL(activated()), ui->consoleInputLineEdit, SLOT(setFocus()));

    // G and S goes to goto entry
    QShortcut* goto_shortcut = new QShortcut(QKeySequence(Qt::Key_G), this);
    connect(goto_shortcut, SIGNAL(activated()), this->omnibar, SLOT(setFocus()));
    QShortcut* seek_shortcut = new QShortcut(QKeySequence(Qt::Key_S), this);
    connect(seek_shortcut, SIGNAL(activated()), this->omnibar, SLOT(setFocus()));

    // : goes to goto entry
    QShortcut* commands_shortcut = new QShortcut(QKeySequence(Qt::Key_Colon), this);
    connect(commands_shortcut, SIGNAL(activated()), this->omnibar, SLOT(showCommands()));

    connect(&webserverThread, SIGNAL(finished()), this, SLOT(webserverThreadFinished()));
}

MainWindow::~MainWindow() {
    delete ui;
    delete core;
}

void MainWindow::start_web_server() {
    // Start web server
    webserverThread.startServer();
}

void MainWindow::webserverThreadFinished()
{
    core->core->http_up = webserverThread.isStarted() ? R_TRUE : R_FALSE;

    // this is not true anymore, cause the webserver might have been stopped
    //if (core->core->http_up == R_FALSE) {
    //    eprintf("FAILED TO LAUNCH\n");
    //}
}

void MainWindow::adjustColumns(QTreeWidget *tw) {
    int count = tw->columnCount();
    for (int i = 0; i != count; ++i) {
        tw->resizeColumnToContents(i);
    }
}

void MainWindow::appendRow(QTreeWidget *tw, const QString &str, const QString &str2,
                      const QString &str3, const QString &str4, const QString &str5) {
    QTreeWidgetItem *tempItem = new QTreeWidgetItem();
    // Fill dummy hidden column
    tempItem->setText(0,"0");
    tempItem->setText(1,str);
    if (str2!=NULL)
        tempItem->setText(2, str2);
    if (str3!=NULL)
        tempItem->setText(3, str3);
    if (str4!=NULL)
        tempItem->setText(4, str4);
    if (str5!=NULL)
        tempItem->setText(5, str5);
    tw->insertTopLevelItem(0, tempItem);
}

void MainWindow::setWebServerState(bool start)
{
    if (start) {
        webserverThread.startServer();

        // Open web interface on default browser
        // ballessay: well isn't this possible with =H&
        //QString link = "http://localhost:9090/";
        //QDesktopServices::openUrl(QUrl(link));
    } else {
        webserverThread.stopServer();
    }
}

void MainWindow::hideDummyColumns() {
    // UGLY, should be a loop over all treewidgets...
    this->functionsDock->functionsTreeWidget->setColumnHidden(0, true);
    this->importsDock->importsTreeWidget->setColumnHidden(0, true);
    this->symbolsDock->symbolsTreeWidget->setColumnHidden(0, true);
    this->relocsDock->relocsTreeWidget->setColumnHidden(0, true);
    this->stringsDock->stringsTreeWidget->setColumnHidden(0, true);
    this->flagsDock->flagsTreeWidget->setColumnHidden(0, true);
    this->commentsDock->commentsTreeWidget->setColumnHidden(0, true);
}

void MainWindow::setFilename(QString fn) {

    // Add file name to window title
    this->filename = fn;
    this->setWindowTitle("Iaito - " + fn);
}

void MainWindow::showConsoleContextMenu(const QPoint &pt)
{
    // Set console output popup menu
    QMenu *menu = ui->consoleOutputTextEdit->createStandardContextMenu();
    menu->clear();
    menu->addAction(ui->actionClear_ConsoleOutput);
    menu->addAction(ui->actionConsoleSync_with_core);
    ui->actionConsoleSync_with_core->setChecked(true);
    ui->consoleOutputTextEdit->setContextMenuPolicy(Qt::CustomContextMenu);

    menu->exec(ui->consoleOutputTextEdit->mapToGlobal(pt));
    delete menu;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QMessageBox::StandardButton ret = QMessageBox::question(this, "Iaito",
                 "Do you really want to exit?\nSave your project before closing!",
                 QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    //qDebug() << ret;
    if (ret == QMessageBox::Save) {
        QSettings settings("iaito", "iaito");
        settings.setValue("geometry", saveGeometry());
        settings.setValue("size", size());
        settings.setValue("pos", pos());
        settings.setValue("state", saveState());
        core->cmd("Ps " + QFileInfo(this->filename).fileName());
        QString notes = this->notepadDock->notesTextEdit->toPlainText().toUtf8().toBase64();
        //this->add_debug_output(notes);
        this->core->cmd("Pnj " + notes);
        QMainWindow::closeEvent(event);
    } else if (ret == QMessageBox::Discard) {
        QSettings settings("iaito", "iaito");
        settings.setValue("geometry", saveGeometry());
        settings.setValue("size", size());
        settings.setValue("pos", pos());
        settings.setValue("state", saveState());
    } else {
        event->ignore();
    }
}

void MainWindow::readSettings()
{
    QSettings settings("iaito", "iaito");
    QByteArray geo = settings.value("geometry", QByteArray()).toByteArray();
    restoreGeometry(geo);
    QByteArray state = settings.value("state", QByteArray()).toByteArray();
    restoreState(state);
    if (settings.value("dark").toBool()) {
        this->dark();
    }
    this->responsive = settings.value("responsive").toBool();
}

void MainWindow::dark() {
    qApp->setStyleSheet("QPlainTextEdit { background-color: rgb(64, 64, 64); color: rgb(222, 222, 222);} QTextEdit { background-color: rgb(64, 64, 64); color: rgb(222, 222, 222);} ");
    this->memoryDock->switchTheme(true);
    QSettings settings("iaito", "iaito");
    settings.setValue("dark", true);
}

void MainWindow::def_theme() {
    qApp->setStyleSheet("");
    this->memoryDock->switchTheme(false);
    QSettings settings("iaito", "iaito");
    settings.setValue("dark", false);
}

/*
 * Refresh widget functions
 */

void MainWindow::refreshFunctions() {
    this->functionsDock->refreshTree();
}

void MainWindow::refreshComments() {
    this->commentsDock->refreshTree();
}

void MainWindow::refreshFlagspaces() {
    int cur_idx = this->flagsDock->flagspaceCombo->currentIndex();
    if (cur_idx<0)cur_idx = 0;
    this->flagsDock->flagspaceCombo->clear();
    this->flagsDock->flagspaceCombo->addItem("(all)");
    for (auto i : core->getList("flagspaces")) {
        this->flagsDock->flagspaceCombo->addItem(i);
    }
    if (cur_idx>0)
        this->flagsDock->flagspaceCombo->setCurrentIndex(cur_idx);
    refreshFlags();
}

void MainWindow::refreshFlags() {
    QString flagspace = this->flagsDock->flagspaceCombo->currentText();
    this->omnibar->clearFlags();
    if (flagspace == "(all)")
        flagspace = "";

    this->flagsDock->flagsTreeWidget->clear();

    for (auto i: core->getList("flags", flagspace)) {
        QStringList a = i.split (",");
        if (a.length()>3) {
            appendRow(this->flagsDock->flagsTreeWidget, a[1], a[2], a[0], a[3]);
            this->omnibar->fillFlags(a[0]);
        }
        else if (a.length()>2) {
            appendRow(this->flagsDock->flagsTreeWidget, a[1], a[2], a[0], "");
            this->omnibar->fillFlags(a[0]);
        }
    }
    adjustColumns(this->flagsDock->flagsTreeWidget);
    // Set omnibar completer for flags and commands
    this->omnibar->setupCompleter();
}

void MainWindow::updateFrames() {
    if (core == NULL)
        return;

    static bool first_time = true;
    if (first_time) {
        setup_mem();
        this->add_output(" > Adding binary information to notepad");
        notepadDock->setText("# Binary information\n\n" + core->cmd("i") +
                             "\n" + core->cmd("ie") + "\n" + core->cmd("iM") + "\n");
        //first_time = false;
    } else {
        refreshMem("");
    }

    refreshFlagspaces();

    auto spi = QAbstractItemView::ScrollPerItem;
    auto spp = QAbstractItemView::ScrollPerPixel;

    // TODO: make this configurable by the user?
    const bool use_scrollperpixel = true;
    if (use_scrollperpixel) {
        this->flagsDock->flagsTreeWidget->setVerticalScrollMode(spp);
        this->symbolsDock->symbolsTreeWidget->setVerticalScrollMode(spp);
        this->importsDock->importsTreeWidget->setVerticalScrollMode(spp);
        this->functionsDock->functionsTreeWidget->setVerticalScrollMode(spp);
        this->stringsDock->stringsTreeWidget->setVerticalScrollMode(spp);
        this->relocsDock->relocsTreeWidget->setVerticalScrollMode(spp);
        this->memoryDock->xreFromTreeWidget_2->setVerticalScrollMode(spp);
        this->memoryDock->xrefToTreeWidget_2->setVerticalScrollMode(spp);
    } else {
        this->flagsDock->flagsTreeWidget->setVerticalScrollMode(spi);
        this->symbolsDock->symbolsTreeWidget->setVerticalScrollMode(spi);
        this->importsDock->importsTreeWidget->setVerticalScrollMode(spi);
        this->functionsDock->functionsTreeWidget->setVerticalScrollMode(spi);
        this->stringsDock->stringsTreeWidget->setVerticalScrollMode(spi);
        this->relocsDock->relocsTreeWidget->setVerticalScrollMode(spi);
        this->memoryDock->xreFromTreeWidget_2->setVerticalScrollMode(spi);
        this->memoryDock->xrefToTreeWidget_2->setVerticalScrollMode(spi);
    }

    this->functionsDock->fillFunctions();

    this->importsDock->fillImports();

    // FIXME, doesn't work bc it sorts strings, not numbers... sigh
    /*
      Use QListWidgetItem::setData() not the constructor to set your value. Then all will work like you expect it to work.

      int yourIntValue = 123456;
      QListWidgetItem *item = new QListWidgetItem;
      item->setData(Qt::DisplayRole, yourIntValue);
    */
    //this->importsDock->importsTreeWidget->sortByColumn(1, Qt::DescendingOrder);

    adjustColumns(this->importsDock->importsTreeWidget);

    this->relocsDock->relocsTreeWidget->clear();
    for (auto i: core->getList ("bin","relocs")) {
        QStringList pieces = i.split (",");
        if (pieces.length()==3)
            appendRow(this->relocsDock->relocsTreeWidget, pieces[0], pieces[1], pieces[2]);
    }
    adjustColumns(this->relocsDock->relocsTreeWidget);

    this->symbolsDock->fillSymbols();

    this->stringsDock->stringsTreeWidget->clear();
    for (auto i : core->getList ("bin", "strings")) {
        QStringList pieces = i.split (",");
        if (pieces.length () == 2)
            appendRow(this->stringsDock->stringsTreeWidget, pieces[0], pieces[1]);
    }
    adjustColumns(this->stringsDock->stringsTreeWidget);

    this->commentsDock->commentsTreeWidget->clear();
    QList<QList<QString>> comments = this->core->getComments();
    for (QList<QString> comment: comments) {
        /*
        QString name;
        //this->add_debug_output("Comment: " + comment[1] + ": " + comment[0]);
        RAnalFunction *fcn = this->core->functionAt(comment[1].toLongLong(0, 16));
        if (fcn != NULL) {
            name = fcn->name;
        } else {
            name = "";
        }
        */
        QString fcn_name = this->core->cmdFunctionAt(comment[1]);
        appendRow(this->commentsDock->commentsTreeWidget, comment[1], fcn_name, comment[0].remove('"'));
    }
    adjustColumns(this->commentsDock->commentsTreeWidget);

    // Add nested comments
    QMap<QString, QList<QList<QString>>> cmts = this->core->getNestedComments();
    for(auto cmt : cmts.keys()) {
        QTreeWidgetItem *item = new QTreeWidgetItem(this->commentsDock->nestedCommentsTreeWidget);
        item->setText(0, cmt);
        QList<QList<QString>> meow = cmts.value(cmt);
        for (int i = 0; i < meow.size(); ++i) {
            QList<QString> tmp = meow.at(i);
            QTreeWidgetItem *it = new QTreeWidgetItem();
            it->setText(0, tmp[1]);
            it->setText(1, tmp[0].remove('"'));
            item->addChild(it);
        }
        this->commentsDock->nestedCommentsTreeWidget->addTopLevelItem(item);
    }
    adjustColumns(this->commentsDock->nestedCommentsTreeWidget);

    // TODO: FIXME: Remove the check for first_time;
    if (first_time) {
        sectionsWidget->tree->clear();
        int row = 0;
        for (auto i: core->getList("bin","sections")) {
            QStringList a = i.split (",");
            if (a.length()>2) {
                // Fix to work with ARM bins
                //if (a[4].startsWith(".")) {
                if (a[4].contains(".")) {
                    QString addr = a[1];
                    QString addr_end = "0x0"+core->itoa(core->math(a[1]+"+"+a[2]));
                    QString size = QString::number(core->math(a[2]));
                    QString name = a[4];
                    this->sectionsWidget->fillSections(row, name, size, addr, addr_end);

                    // Used to select a color for the sections graph
                    if (row == 10) {
                        row = 0;
                    } else {
                        row++;
                    }
                }
            }
        }
        //adjustColumns(sectionsWidget->tree);
        sectionsWidget->adjustColumns();

        first_time = false;

        this->dashboardDock->updateContents();
    }
}

/*
 * End of refresh widget functions
 */

void MainWindow::on_actionLock_triggered()
{
    doLock = !doLock;
    if (doLock) {
    foreach (QDockWidget *dockWidget, findChildren<QDockWidget *>()) {
        dockWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
    }
    } else {
    foreach (QDockWidget *dockWidget, findChildren<QDockWidget *>()) {
        dockWidget->setFeatures(QDockWidget::AllDockWidgetFeatures);
    }
    }
}

void MainWindow::lockUnlock_Docks(bool what)
{
    if(what) {
        foreach (QDockWidget *dockWidget, findChildren<QDockWidget *>()) {
            dockWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
        }
    } else {
        foreach (QDockWidget *dockWidget, findChildren<QDockWidget *>()) {
            dockWidget->setFeatures(QDockWidget::AllDockWidgetFeatures);
        }
    }

}

void MainWindow::on_actionLockUnlock_triggered()
{
    if(ui->actionLockUnlock->isChecked())
    {
        foreach (QDockWidget *dockWidget, findChildren<QDockWidget *>()) {
            dockWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
        }
        ui->actionLockUnlock->setIcon( QIcon(":/new/prefix1/lock") );
    } else {
        foreach (QDockWidget *dockWidget, findChildren<QDockWidget *>()) {
            dockWidget->setFeatures(QDockWidget::AllDockWidgetFeatures);
        }
        ui->actionLockUnlock->setIcon( QIcon(":/new/prefix1/unlock") );
    }
}

void MainWindow::on_actionTabs_triggered()
{
    if (ui->centralTabWidget->tabPosition() == QTabWidget::South) {
        ui->centralTabWidget->setTabPosition(QTabWidget::North);
        this->memoryDock->memTabWidget->setTabPosition(QTabWidget::North);
        this->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);
    } else {
        ui->centralTabWidget->setTabPosition(QTabWidget::South);
        this->memoryDock->memTabWidget->setTabPosition(QTabWidget::South);
        this->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::South);
    }
}

void MainWindow::on_actionMem_triggered()
{
    //this->memoryDock->show();
    //this->memoryDock->raise();
    MemoryWidget* newMemDock = new MemoryWidget(this);
    this->dockList << newMemDock;
    newMemDock->setAttribute(Qt::WA_DeleteOnClose);
    this->tabifyDockWidget(this->memoryDock, newMemDock);
    newMemDock->refreshDisasm("");
    newMemDock->refreshHexdump("");
}

void MainWindow::on_actionFunctions_triggered()
{
    if (this->functionsDock->isVisible()) {
        this->functionsDock->close();
    } else {
        this->functionsDock->show();
        this->functionsDock->raise();
    }
}

void MainWindow::on_actionImports_triggered()
{
    if (this->importsDock->isVisible()) {
        this->importsDock->close();
    } else {
        this->importsDock->show();
        this->importsDock->raise();
    }
}

void MainWindow::on_actionSymbols_triggered()
{
    if (this->symbolsDock->isVisible()) {
        this->symbolsDock->close();
    } else {
        this->symbolsDock->show();
        this->symbolsDock->raise();
    }
}

void MainWindow::on_actionReloc_triggered()
{
    if (this->relocsDock->isVisible()) {
        this->relocsDock->close();
    } else {
        this->relocsDock->show();
        this->relocsDock->raise();
    }
}

void MainWindow::on_actionStrings_triggered()
{
    if (this->stringsDock->isVisible()) {
        this->stringsDock->close();
    } else {
        this->stringsDock->show();
        this->stringsDock->raise();
    }
}

void MainWindow::on_actionSections_triggered()
{
    if (this->sectionsDock->isVisible()) {
        this->sectionsDock->close();
    } else {
        this->sectionsDock->show();
        this->sectionsDock->raise();
    }
}

void MainWindow::on_actionFlags_triggered()
{
    if (this->flagsDock->isVisible()) {
        this->flagsDock->close();
    } else {
        this->flagsDock->show();
        this->flagsDock->raise();
    }
}

void MainWindow::on_actionComents_triggered()
{
    if (this->commentsDock->isVisible()) {
        this->commentsDock->close();
    } else {
        this->commentsDock->show();
        this->commentsDock->raise();
    }
}

void MainWindow::on_actionNotepad_triggered()
{
    if (this->notepadDock->isVisible()) {
        this->notepadDock->close();
    } else {
        this->notepadDock->show();
        this->notepadDock->raise();
    }
}

void MainWindow::on_actionAbout_triggered()
{
    AboutDialog* a = new AboutDialog(this);
    a->open();
}

void MainWindow::on_consoleInputLineEdit_returnPressed()
{
    if (this->core) {
        QString input = ui->consoleInputLineEdit->text();
        ui->consoleOutputTextEdit->appendPlainText(this->core->cmd(input));
        ui->consoleOutputTextEdit->verticalScrollBar()->setValue(ui->consoleOutputTextEdit->verticalScrollBar()->maximum());
        // Add new command to history
        QCompleter *completer = ui->consoleInputLineEdit->completer();
        /*
         * TODO: FIXME: Crashed the fucking app
         * ballessay: yes this will crash if no completer is set -> nullptr
         */
        //QStringListModel *completerModel = (QStringListModel*)(completer->model());
        //completerModel->setStringList(completerModel->stringList() << input);
        ui->consoleInputLineEdit->setText("");
        // TODO: add checkbox to enable/disable updating the whole ui or just update the list widgets, not disasm/hex
        //this->updateFrames();
    }
}

void MainWindow::on_showHistoToolButton_clicked()
{
    if (ui->showHistoToolButton->isChecked()) {
        QCompleter *completer = ui->consoleInputLineEdit->completer();
        completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
        completer->complete();
    } else {
        QCompleter *completer = ui->consoleInputLineEdit->completer();
        completer->setCompletionMode(QCompleter::PopupCompletion);
    }
}

void MainWindow::on_actionClear_ConsoleOutput_triggered()
{
    ui->consoleOutputTextEdit->setPlainText("");
}

void MainWindow::on_actionRefresh_Panels_triggered()
{
    this->updateFrames();
}

void MainWindow::seek(const QString& offset, const QString& name) {
    if (offset.length()==0)
        return;
    if (name != NULL)
        this->memoryDock->setWindowTitle(name);
    this->hexdumpTopOffset = 0;
    this->hexdumpBottomOffset = 0;
    core->seek (offset);

    refreshMem(offset);
}

void MainWindow::setup_mem() {
    QString off = this->core->cmd("afo entry0").trimmed();
    //graphicsBar->refreshColorBar();
    graphicsBar->fillData();
    this->memoryDock->refreshDisasm(off);
    this->memoryDock->refreshHexdump(off);
    this->memoryDock->create_graph(off);
    this->memoryDock->get_refs_data(off);
    this->memoryDock->setFcnName(off);
}

void MainWindow::refreshMem(QString off) {
    //add_debug_output("Refreshing to: " + off);
    //graphicsBar->refreshColorBar();
    this->memoryDock->refreshDisasm(off);
    this->memoryDock->refreshHexdump(off);
    this->memoryDock->create_graph(off);
    this->memoryDock->get_refs_data(off);
    this->memoryDock->setFcnName(off);
}

void MainWindow::on_backButton_clicked()
{
    this->core->cmd("s-");
    QString back_offset = this->core->cmd("s=").split(" > ").last().trimmed();
    if (back_offset != "") {
        this->add_debug_output(back_offset);
        this->seek(back_offset);
    }
}

void MainWindow::on_actionCalculator_triggered()
{
    if (!this->sideBar->isVisible()) {
        this->on_actionShow_Hide_mainsidebar_triggered();
    }
}

void MainWindow::on_actionCreate_File_triggered()
{
    createNewDialog* n = new createNewDialog(this);
    n->exec();
}

void MainWindow::on_actionAssembler_triggered()
{
    if (!this->sideBar->isVisible()) {
        this->on_actionShow_Hide_mainsidebar_triggered();
    }
}

void MainWindow::on_consoleExecButton_clicked()
{
    on_consoleInputLineEdit_returnPressed();
}

void MainWindow::on_actionStart_Web_Server_triggered()
{
    setWebServerState(ui->actionStart_Web_Server->isChecked());
}

void MainWindow::on_actionConsoleSync_with_core_triggered()
{
    if (ui->actionConsoleSync_with_core->isChecked()) {
        //Enable core syncronization
    } else {
        // Disable core sync
    }
}

void MainWindow::on_actionDisasAdd_comment_triggered()
{
    CommentsDialog* c = new CommentsDialog(this);
    c->exec();
}

void MainWindow::restoreDocks()
{
    addDockWidget(Qt::RightDockWidgetArea, sectionsDock);
    addDockWidget(Qt::TopDockWidgetArea, this->dashboardDock);
    this->tabifyDockWidget(sectionsDock, this->commentsDock);
    this->tabifyDockWidget(this->dashboardDock, this->memoryDock);
    this->tabifyDockWidget(this->dashboardDock, this->functionsDock);
    this->tabifyDockWidget(this->dashboardDock, this->flagsDock);
    this->tabifyDockWidget(this->dashboardDock, this->stringsDock);
    this->tabifyDockWidget(this->dashboardDock, this->relocsDock);
    this->tabifyDockWidget(this->dashboardDock, this->importsDock);
    this->tabifyDockWidget(this->dashboardDock, this->symbolsDock);
    this->tabifyDockWidget(this->dashboardDock, this->notepadDock);
    this->dashboardDock->raise();
    sectionsDock->raise();
    this->functionsDock->raise();
}

void MainWindow::on_actionDefaut_triggered()
{
    hideAllDocks();
    restoreDocks();
    showDefaultDocks();
    this->dashboardDock->raise();
}

void MainWindow::hideAllDocks()
{
    sectionsDock->hide();
    this->functionsDock->hide();
    this->memoryDock->hide();
    this->commentsDock->hide();
    this->flagsDock->hide();
    this->stringsDock->hide();
    this->relocsDock->hide();
    this->importsDock->hide();
    this->symbolsDock->hide();
    this->notepadDock->hide();
    this->dashboardDock->hide();
}

void MainWindow::showDefaultDocks()
{
    sectionsDock->show();
    this->functionsDock->show();
    this->memoryDock->show();
    this->commentsDock->show();
    this->stringsDock->show();
    this->importsDock->show();
    this->symbolsDock->show();
    this->notepadDock->show();
    this->dashboardDock->show();
}

void MainWindow::on_actionhide_bottomPannel_triggered()
{
    if (ui->centralWidget->isVisible()) {
        ui->centralWidget->hide();
    } else {
        ui->centralWidget->show();
    }
}

void MainWindow::send_to_notepad(QString txt)
{
    this->notepadDock->notesTextEdit->appendPlainText("```\n" + txt + "\n```");
}

void MainWindow::on_actionFunctionsRename_triggered()
{
    RenameDialog* r = new RenameDialog(this);
    // Get function based on click position
    //r->setFunctionName(fcn_name);
    r->open();
}

void MainWindow::get_refs(const QString& offset)
{
    this->memoryDock->get_refs_data(offset);
}

void MainWindow::add_output(QString msg)
{
    ui->consoleOutputTextEdit->appendPlainText(msg);
    ui->consoleOutputTextEdit->verticalScrollBar()->setValue(ui->consoleOutputTextEdit->verticalScrollBar()->maximum());
}

void MainWindow::add_debug_output(QString msg)
{
    ui->consoleOutputTextEdit->appendHtml("<font color=\"red\"> [DEBUG]:\t" + msg + "</font>");
    ui->consoleOutputTextEdit->verticalScrollBar()->setValue(ui->consoleOutputTextEdit->verticalScrollBar()->maximum());
}

void MainWindow::on_actionNew_triggered()
{
    close();
    on_actionLoad_triggered();
}

void MainWindow::on_actionSave_triggered()
{
    core->cmd("Ps " + QFileInfo(this->filename).fileName());
    QString notes = this->notepadDock->notesTextEdit->toPlainText().toUtf8().toBase64();
    //this->add_debug_output(notes);
    this->core->cmd("Pnj " + notes);
    this->add_output("Project saved");
}

void MainWindow::on_actionRun_Script_triggered()
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setViewMode(QFileDialog::Detail);
    dialog.setDirectory(QDir::home());

    QString fileName;
    fileName = dialog.getOpenFileName(this, "Select radare2 script");

    qDebug() << "Meow: " + fileName;
    this->core->cmd(". " + fileName);
    this->refreshMem("");
}

void MainWindow::on_actionDark_Theme_triggered()
{
    this->dark();
}

void MainWindow::on_actionWhite_Theme_triggered()
{
    this->def_theme();
}

void MainWindow::on_actionSDB_browser_triggered()
{
    this->sdbDock = new SdbDock(this);
    this->tabifyDockWidget(this->memoryDock, this->sdbDock);
    this->sdbDock->setFloating(true);
    this->sdbDock->show();
}

void MainWindow::on_actionLoad_triggered()
{
    QProcess process(this);
    process.setEnvironment(QProcess::systemEnvironment());
    process.startDetached(qApp->applicationFilePath());
}

void MainWindow::on_actionShow_Hide_mainsidebar_triggered()
{
    if (ui->sideToolBar->isVisible()) {
        ui->sideToolBar->hide();
    } else {
        ui->sideToolBar->show();
    }
}

void MainWindow::on_actionDashboard_triggered()
{
    if (this->dashboardDock->isVisible()) {
        this->dashboardDock->close();
    } else {
        this->dashboardDock->show();
        this->dashboardDock->raise();
    }
}

void MainWindow::showSectionsContextMenu(const QPoint &pt)
{
    // Set functions popup menu
    QMenu *menu = new QMenu(this->sectionsDock);
    menu->clear();
    menu->addAction(ui->actionSectionsHorizontal);
    menu->addAction(ui->actionSectionsVertical);

    if (this->sectionsWidget->orientation() == 1) {
        ui->actionSectionsHorizontal->setChecked(true);
        ui->actionSectionsVertical->setChecked(false);
    } else {
        ui->actionSectionsVertical->setChecked(true);
        ui->actionSectionsHorizontal->setChecked(false);
    }

    this->sectionsDock->setContextMenuPolicy(Qt::CustomContextMenu);
    menu->exec(this->sectionsDock->mapToGlobal(pt));
    delete menu;
}

void MainWindow::on_actionSectionsHorizontal_triggered()
{
    this->sectionsWidget->setOrientation(Qt::Horizontal);
}

void MainWindow::on_actionSectionsVertical_triggered()
{
    this->sectionsWidget->setOrientation(Qt::Vertical);
}

void MainWindow::on_actionForward_triggered()
{
    this->core->cmd("s+");
    QString offset = this->core->cmd("s=").split(" > ").last().trimmed();
    if (offset != "") {
        this->add_debug_output(offset);
        this->seek(offset);
    }
}

void MainWindow::toggleResponsive(bool maybe) {
    this->responsive = maybe;
    // Save options in settings
    QSettings settings("iaito", "iaito");
    settings.setValue("responsive", this->responsive);
}

void MainWindow::on_actionTabs_on_Top_triggered()
{
    this->on_actionTabs_triggered();
}

void MainWindow::on_actionReset_settings_triggered()
{
    QMessageBox::StandardButton ret = QMessageBox::question(this, "Iaito",
                 "Do you really want to clear all settings?",
                 QMessageBox::Ok | QMessageBox::Cancel);
    if (ret == QMessageBox::Ok) {
        // Save options in settings
        QSettings settings("iaito", "iaito");
        settings.clear();
    }
}

void MainWindow::on_actionQuit_triggered()
{
    close();
}
