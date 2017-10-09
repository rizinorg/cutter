#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "dialogs/CreateNewDialog.h"
#include "dialogs/CommentsDialog.h"
#include "dialogs/AboutDialog.h"
#include "dialogs/RenameDialog.h"
#include "dialogs/AsmOptionsDialog.h"
#include "utils/Helpers.h"

#include <QComboBox>
#include <QCompleter>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QFontDialog>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QMessageBox>
#include <QProcess>
#include <QPropertyAnimation>
#include <QScrollBar>
#include <QSettings>
#include <QShortcut>
#include <QStringListModel>
#include <QStyledItemDelegate>
#include <QStyleFactory>
#include <QTextCursor>
#include <QtGlobal>
#include <QToolButton>
#include <QToolTip>
#include <QTreeWidgetItem>

#include "utils/Highlighter.h"
#include "utils/HexAsciiHighlighter.h"
#include "utils/Helpers.h"
#include "dialogs/NewFileDialog.h"

#include "widgets/MemoryWidget.h"
#include "widgets/FunctionsWidget.h"
#include "widgets/SectionsWidget.h"
#include "widgets/CommentsWidget.h"
#include "widgets/ImportsWidget.h"
#include "widgets/ExportsWidget.h"
#include "widgets/SymbolsWidget.h"
#include "widgets/StringsWidget.h"
#include "widgets/SectionsDock.h"
#include "widgets/RelocsWidget.h"
#include "widgets/FlagsWidget.h"
#include "widgets/CodeGraphic.h"
#include "widgets/Dashboard.h"
#include "widgets/Notepad.h"
#include "widgets/Sidebar.h"
#include "widgets/SdbDock.h"
#include "widgets/Omnibar.h"
#include "widgets/ConsoleWidget.h"
#include "Settings.h"
#include "dialogs/OptionsDialog.h"
#include "widgets/EntrypointWidget.h"
#include "widgets/DisassemblerGraphView.h"

// graphics
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include <QGraphicsView>

#include <cassert>

static void registerCustomFonts()
{
    int ret = QFontDatabase::addApplicationFont(":/fonts/Anonymous Pro.ttf");
    assert(-1 != ret && "unable to register Anonymous Pro.ttf");

    ret = QFontDatabase::addApplicationFont(":/fonts/Inconsolata-Regular.ttf");
    assert(-1 != ret && "unable to register Inconsolata-Regular.ttf");

    // do not issue a warning in release
    Q_UNUSED(ret)
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    core(CutterCore::getInstance()),
    memoryDock(nullptr),
    notepadDock(nullptr),
    asmDock(nullptr),
    calcDock(nullptr),
    omnibar(nullptr),
    sideBar(nullptr),
    ui(new Ui::MainWindow),
    highlighter(nullptr),
    hex_highlighter(nullptr),
    graphicsBar(nullptr),
    entrypointDock(nullptr),
    functionsDock(nullptr),
    importsDock(nullptr),
    exportsDock(nullptr),
    symbolsDock(nullptr),
    relocsDock(nullptr),
    commentsDock(nullptr),
    stringsDock(nullptr),
    flagsDock(nullptr),
    dashboardDock(nullptr),
    gotoEntry(nullptr),
    sdbDock(nullptr),
    sidebar_action(nullptr),
    sectionsDock(nullptr),
    consoleWidget(nullptr),
    webserver(core)
{
    doLock = false;
}

MainWindow::~MainWindow()
{
    delete core;
}

void MainWindow::initUI()
{
    ui->setupUi(this);

    registerCustomFonts();

    /*
    * Toolbar
    */
    // Hide central tab widget tabs
    QTabBar *centralbar = ui->centralTabWidget->tabBar();
    centralbar->setVisible(false);
    consoleWidget = new ConsoleWidget(this);
    ui->tabVerticalLayout->addWidget(consoleWidget);

    // Sepparator between back/forward and undo/redo buttons
    QWidget *spacer4 = new QWidget();
    spacer4->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    spacer4->setMinimumSize(10, 10);
    ui->mainToolBar->insertWidget(ui->actionForward, spacer4);

    // Popup menu on theme toolbar button
    QToolButton *backButton = new QToolButton(this);
    backButton->setIcon(QIcon(":/img/icons/arrow_left.svg"));
    //backButton->setPopupMode(QToolButton::DelayedPopup);
    ui->mainToolBar->insertWidget(ui->actionForward, backButton);
    connect(backButton, SIGNAL(clicked()), this, SLOT(backButton_clicked()));

    // Sepparator between undo/redo and goto lineEdit
    QWidget *spacer3 = new QWidget();
    spacer3->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    spacer3->setMinimumSize(20, 20);
    spacer3->setMaximumWidth(300);
    ui->mainToolBar->insertWidget(ui->actionShow_Hide_mainsidebar, spacer3);

    // Omnibar LineEdit
    this->omnibar = new Omnibar(this);
    ui->mainToolBar->insertWidget(ui->actionShow_Hide_mainsidebar, this->omnibar);

    // Add special separators to the toolbar that expand to separate groups of elements
    QWidget *spacer2 = new QWidget();
    spacer2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    spacer2->setMinimumSize(10, 10);
    spacer2->setMaximumWidth(300);
    ui->mainToolBar->insertWidget(ui->actionShow_Hide_mainsidebar, spacer2);

    // Separator between back/forward and undo/redo buttons
    QWidget *spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    spacer->setMinimumSize(20, 20);
    ui->mainToolBar->addWidget(spacer);

    // codeGraphics tool bar
    this->graphicsBar = new GraphicsBar(this);
    this->graphicsBar->setMovable(false);
    addToolBarBreak(Qt::TopToolBarArea);
    addToolBar(graphicsBar);

    /*
     * Dock Widgets
     */
    dockWidgets.reserve(12);

    // Add Memory DockWidget
    this->memoryDock = new MemoryWidget();
    dockWidgets.push_back(memoryDock);
    // To use in the future when we handle more than one memory views
    // this->memoryDock->setAttribute(Qt::WA_DeleteOnClose);
    // this->add_debug_output( QString::number(this->dockList.length()) );

    // Add Sections dock panel
    this->sectionsDock = new SectionsDock(this);
    dockWidgets.push_back(sectionsDock);

    // Add entrypoint DockWidget
    this->entrypointDock = new EntrypointWidget(this);
    dockWidgets.push_back(entrypointDock);

    // Add functions DockWidget
    this->functionsDock = new FunctionsWidget(this);
    dockWidgets.push_back(functionsDock);

    // Add imports DockWidget
    this->importsDock = new ImportsWidget(this);
    dockWidgets.push_back(importsDock);

    // Add exports DockWidget
    this->exportsDock = new ExportsWidget(this);
    dockWidgets.push_back(exportsDock);

    // Add symbols DockWidget
    this->symbolsDock = new SymbolsWidget(this);
    dockWidgets.push_back(symbolsDock);

    // Add relocs DockWidget
    this->relocsDock = new RelocsWidget(this);
    dockWidgets.push_back(relocsDock);

    // Add comments DockWidget
    this->commentsDock = new CommentsWidget(this);
    dockWidgets.push_back(commentsDock);

    // Add strings DockWidget
    this->stringsDock = new StringsWidget(this);
    dockWidgets.push_back(stringsDock);

    // Add flags DockWidget
    this->flagsDock = new FlagsWidget(this);
    dockWidgets.push_back(flagsDock);

    // Add Notepad Dock panel
    this->notepadDock = new Notepad(this);
    dockWidgets.push_back(notepadDock);
    connect(memoryDock, SIGNAL(fontChanged(QFont)), notepadDock, SLOT(setFonts(QFont)));

    //Add Dashboard Dock panel
    this->dashboardDock = new Dashboard(this);
    dockWidgets.push_back(dashboardDock);

    // Set up dock widgets default layout
    restoreDocks();
    hideAllDocks();
    showDefaultDocks();

    // Restore saved settings
    this->readSettings();
    // TODO: Allow the user to select this option visually in the GUI settings
    // Adjust the DockWidget areas
    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    //setCorner( Qt::TopRightCorner, Qt::RightDockWidgetArea );
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    //setCorner( Qt::BottomRightCorner, Qt::RightDockWidgetArea );

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
    QShortcut *cmd_shortcut = new QShortcut(QKeySequence(Qt::Key_Period), this);
    connect(cmd_shortcut, SIGNAL(activated()), consoleWidget, SLOT(focusInputLineEdit()));

    // G and S goes to goto entry
    QShortcut *goto_shortcut = new QShortcut(QKeySequence(Qt::Key_G), this);
    connect(goto_shortcut, SIGNAL(activated()), this->omnibar, SLOT(setFocus()));
    QShortcut *seek_shortcut = new QShortcut(QKeySequence(Qt::Key_S), this);
    connect(seek_shortcut, SIGNAL(activated()), this->omnibar, SLOT(setFocus()));

    // : goes to goto entry
    QShortcut *commands_shortcut = new QShortcut(QKeySequence(Qt::Key_Colon), this);
    connect(commands_shortcut, SIGNAL(activated()), this->omnibar, SLOT(showCommands()));

    QShortcut *refresh_shortcut = new QShortcut(QKeySequence(QKeySequence::Refresh), this);
    connect(refresh_shortcut, SIGNAL(activated()), this, SLOT(refreshVisibleDockWidgets()));
}

void MainWindow::openFile(const QString &fn, int anal_level, QList<QString> advanced)
{
    QString project_name = qhelpers::uniqueProjectName(fn);

    if (core->getProjectNames().contains(project_name))
        openProject(project_name);
    else
        openNewFile(fn, anal_level, advanced);
}

void MainWindow::openNewFile(const QString &fn, int anal_level, QList<QString> advanced)
{
    setFilename(fn);

    OptionsDialog *o = new OptionsDialog(this);
    o->setAttribute(Qt::WA_DeleteOnClose);
    o->show();

    if (anal_level >= 0)
        o->setupAndStartAnalysis(anal_level, advanced);
}

void MainWindow::openProject(const QString &project_name)
{
    QString filename = core->cmd("Pi " + project_name);
    setFilename(filename.trimmed());

    core->cmd("Po " + project_name);

    initUI();
    finalizeOpen();
}

void MainWindow::finalizeOpen()
{
    core->getOpcodes();

    // Set settings to override any incorrect saved in the project
    core->setSettings();


    addOutput(tr(" > Populating UI"));
    // FIXME: initialization order frakup. the next line is needed so that the
    // comments widget displays the function names.
    core->cmd("fs sections");
    updateFrames();

    memoryDock->selectHexPreview();

    // Restore project notes
    QString notes = this->core->cmd("Pnj");
    //qDebug() << "Notes:" << notes;
    if (notes != "")
    {
        QByteArray ba;
        ba.append(notes);
        notepadDock->setText(QByteArray::fromBase64(ba));
    }
    else
    {
        addOutput(tr(" > Adding binary information to notepad"));

        notepadDock->setText(tr("# Binary information\n\n") + core->cmd("i") +
                             "\n" + core->cmd("ie") + "\n" + core->cmd("iM") + "\n");
    }

    //Get binary beginning/end addresses
    this->core->binStart = this->core->cmd("?v $M");
    this->core->binEnd = this->core->cmd("?v $M+$s");

    addOutput(tr(" > Finished, happy reversing :)"));
    // Add fortune message
    addOutput("\n" + core->cmd("fo"));
    memoryDock->setWindowTitle("entry0");
    start_web_server();
    showMaximized();
    // Initialize syntax highlighters
    memoryDock->highlightDisasms();
    notepadDock->highlightPreview();
}

void MainWindow::saveProject()
{
    QString project_name = qhelpers::uniqueProjectName(filename);
    core->cmd("Ps " + project_name);
    QString notes = this->notepadDock->textToBase64();
    //this->add_debug_output(notes);
    this->core->cmd("Pnj " + notes);
    this->addOutput(tr("Project saved: ") + project_name);
}

void MainWindow::start_web_server()
{
    // Start web server
    webserver.start();
}


void MainWindow::setWebServerState(bool start)
{
    if (start)
    {
        webserver.start();

        // Open web interface on default browser
        // ballessay: well isn't this possible with =H&
        //QString link = "http://localhost:9090/";
        //QDesktopServices::openUrl(QUrl(link));
    }
    else
    {
        webserver.stop();
    }
}

void MainWindow::raiseMemoryDock()
{
    memoryDock->raise();
}

void MainWindow::toggleSideBarTheme()
{
    sideBar->themesButtonToggle();
}

void MainWindow::refreshOmniBar(const QStringList &flags)
{
    omnibar->refresh(flags);
}

void MainWindow::setFilename(const QString &fn)
{
    // Add file name to window title
    this->filename = fn;
    this->setWindowTitle(APPNAME" - " + fn);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QMessageBox::StandardButton ret = QMessageBox::question(this, APPNAME,
                                      tr("Do you really want to exit?\nSave your project before closing!"),
                                      (QMessageBox::StandardButtons)(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel));
    //qDebug() << ret;
    if (ret == QMessageBox::Save)
    {
        QSettings settings;
        settings.setValue("geometry", saveGeometry());
        settings.setValue("size", size());
        settings.setValue("pos", pos());
        settings.setValue("state", saveState());
        saveProject();
        QMainWindow::closeEvent(event);
    }
    else if (ret == QMessageBox::Discard)
    {
        QSettings settings;
        settings.setValue("geometry", saveGeometry());
        settings.setValue("size", size());
        settings.setValue("pos", pos());
        settings.setValue("state", saveState());
    }
    else
    {
        event->ignore();
    }
}

void MainWindow::readSettings()
{
    QSettings settings;
    QByteArray geo = settings.value("geometry", QByteArray()).toByteArray();
    restoreGeometry(geo);
    QByteArray state = settings.value("state", QByteArray()).toByteArray();
    restoreState(state);
    if (settings.value("dark").toBool())
    {
        this->dark();
    }
    this->responsive = settings.value("responsive").toBool();
}

void MainWindow::dark()
{
    qApp->setStyleSheet("QPlainTextEdit { background-color: rgb(64, 64, 64); color: rgb(222, 222, 222);} QTextEdit { background-color: rgb(64, 64, 64); color: rgb(222, 222, 222);} ");
    this->memoryDock->switchTheme(true);
    QSettings settings;
    settings.setValue("dark", true);
}

void MainWindow::def_theme()
{
    qApp->setStyleSheet("");
    this->memoryDock->switchTheme(false);
    QSettings settings;
    settings.setValue("dark", false);
}

/*
 * Refresh widget functions
 */

void MainWindow::refreshFunctions()
{
    functionsDock->refresh();
}

void MainWindow::refreshComments()
{
    commentsDock->refresh();
}

void MainWindow::updateFrames()
{
    if (core == NULL)
        return;

    static bool first_time = true;

    if (first_time)
    {
        for (auto w : dockWidgets)
        {
            w->setup();
        }

        first_time = false;
    }
    else
    {
        for (auto w : dockWidgets)
        {
            w->refresh();
        }
    }

    // graphicsBar->refreshColorBar();
    graphicsBar->fillData();
}

void MainWindow::on_actionLock_triggered()
{
    doLock = !doLock;
    if (doLock)
    {
        foreach (QDockWidget *dockWidget, findChildren<QDockWidget *>())
        {
            dockWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
        }
    }
    else
    {
        foreach (QDockWidget *dockWidget, findChildren<QDockWidget *>())
        {
            dockWidget->setFeatures(QDockWidget::AllDockWidgetFeatures);
        }
    }
}

void MainWindow::lockUnlock_Docks(bool what)
{
    if (what)
    {
        foreach (QDockWidget *dockWidget, findChildren<QDockWidget *>())
        {
            dockWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
        }
    }
    else
    {
        foreach (QDockWidget *dockWidget, findChildren<QDockWidget *>())
        {
            dockWidget->setFeatures(QDockWidget::AllDockWidgetFeatures);
        }
    }

}

void MainWindow::on_actionLockUnlock_triggered()
{
    if (ui->actionLockUnlock->isChecked())
    {
        foreach (QDockWidget *dockWidget, findChildren<QDockWidget *>())
        {
            dockWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
        }
        ui->actionLockUnlock->setIcon(QIcon(":/lock"));
    }
    else
    {
        foreach (QDockWidget *dockWidget, findChildren<QDockWidget *>())
        {
            dockWidget->setFeatures(QDockWidget::AllDockWidgetFeatures);
        }
        ui->actionLockUnlock->setIcon(QIcon(":/unlock"));
    }
}

void MainWindow::on_actionTabs_triggered()
{
    if (ui->centralTabWidget->tabPosition() == QTabWidget::South)
    {
        ui->centralTabWidget->setTabPosition(QTabWidget::North);
        this->memoryDock->memTabWidget->setTabPosition(QTabWidget::North);
        this->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);
    }
    else
    {
        ui->centralTabWidget->setTabPosition(QTabWidget::South);
        this->memoryDock->memTabWidget->setTabPosition(QTabWidget::South);
        this->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::South);
    }
}

void MainWindow::on_actionMem_triggered()
{
    //this->memoryDock->show();
    //this->memoryDock->raise();
    MemoryWidget *newMemDock = new MemoryWidget();
    this->dockWidgets << newMemDock;
    newMemDock->setAttribute(Qt::WA_DeleteOnClose);
    this->tabifyDockWidget(this->memoryDock, newMemDock);
    newMemDock->refreshDisasm();
    newMemDock->refreshHexdump();
}

void MainWindow::on_actionEntry_points_triggered()
{
    toggleDockWidget(entrypointDock);
}

void MainWindow::on_actionFunctions_triggered()
{
    toggleDockWidget(functionsDock);
}

void MainWindow::on_actionImports_triggered()
{
    toggleDockWidget(importsDock);
}

void MainWindow::on_actionExports_triggered()
{
    toggleDockWidget(exportsDock);
}

void MainWindow::on_actionSymbols_triggered()
{
    toggleDockWidget(symbolsDock);
}

void MainWindow::on_actionReloc_triggered()
{
    toggleDockWidget(relocsDock);
}

void MainWindow::on_actionStrings_triggered()
{
    toggleDockWidget(stringsDock);
}

void MainWindow::on_actionSections_triggered()
{
    toggleDockWidget(sectionsDock);
}

void MainWindow::on_actionFlags_triggered()
{
    toggleDockWidget(flagsDock);
}

void MainWindow::on_actionComents_triggered()
{
    toggleDockWidget(commentsDock);
}

void MainWindow::on_actionNotepad_triggered()
{
    toggleDockWidget(notepadDock);
}

void MainWindow::on_actionAbout_triggered()
{
    AboutDialog *a = new AboutDialog(this);
    a->open();
}

void MainWindow::on_actionRefresh_Panels_triggered()
{
    this->updateFrames();
}

void MainWindow::toggleDockWidget(DockWidget *dock_widget)
{
    if (dock_widget->isVisible())
    {
        dock_widget->close();
    }
    else
    {
        dock_widget->show();
        dock_widget->raise();
    }
}

void MainWindow::setCursorAddress(RVA addr)
{
    this->cursorAddress = addr;
    emit cursorAddressChanged(core->getOffset());
}

void MainWindow::backButton_clicked()
{
    QList<RVA> seek_history = core->getSeekHistory();
    this->core->cmd("s-");
    RVA offset = this->core->getOffset();
    //QString fcn = this->core->cmdFunctionAt(QString::number(offset));
    core->seek(offset);
}

void MainWindow::on_actionCalculator_triggered()
{
    if (!this->sideBar->isVisible())
    {
        this->on_actionShow_Hide_mainsidebar_triggered();
    }
}

void MainWindow::on_actionCreate_File_triggered()
{
    CreateNewDialog *n = new CreateNewDialog(this);
    n->exec();
}

void MainWindow::on_actionAssembler_triggered()
{
    if (!this->sideBar->isVisible())
    {
        this->on_actionShow_Hide_mainsidebar_triggered();
    }
}

void MainWindow::on_actionStart_Web_Server_triggered()
{
    setWebServerState(ui->actionStart_Web_Server->isChecked());
}

void MainWindow::on_actionDisasAdd_comment_triggered()
{
    CommentsDialog *c = new CommentsDialog(this);
    c->exec();
}

void MainWindow::restoreDocks()
{
    addDockWidget(Qt::RightDockWidgetArea, sectionsDock);
    addDockWidget(Qt::TopDockWidgetArea, this->dashboardDock);
    this->tabifyDockWidget(sectionsDock, this->commentsDock);
    this->tabifyDockWidget(this->dashboardDock, this->memoryDock);
    this->tabifyDockWidget(this->dashboardDock, this->entrypointDock);
    this->tabifyDockWidget(this->dashboardDock, this->functionsDock);
    this->tabifyDockWidget(this->dashboardDock, this->flagsDock);
    this->tabifyDockWidget(this->dashboardDock, this->stringsDock);
    this->tabifyDockWidget(this->dashboardDock, this->relocsDock);
    this->tabifyDockWidget(this->dashboardDock, this->importsDock);
    this->tabifyDockWidget(this->dashboardDock, this->exportsDock);
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
    for (auto w : dockWidgets)
    {
        w->hide();
    }
}

void MainWindow::showDefaultDocks()
{
    const QList<DockWidget *> defaultDocks = { sectionsDock,
                                               entrypointDock,
                                               functionsDock,
                                               memoryDock,
                                               commentsDock,
                                               stringsDock,
                                               importsDock,
                                               symbolsDock,
                                               notepadDock,
                                               dashboardDock
                                             };

    for (auto w : dockWidgets)
    {
        if (defaultDocks.contains(w))
        {
            w->show();
        }
    }
}

void MainWindow::on_actionhide_bottomPannel_triggered()
{
    if (ui->centralWidget->isVisible())
    {
        ui->centralWidget->hide();
    }
    else
    {
        ui->centralWidget->show();
    }
}

void MainWindow::sendToNotepad(const QString &txt)
{
    this->notepadDock->appendPlainText("```\n" + txt + "\n```");
}

void MainWindow::on_actionFunctionsRename_triggered()
{
    RenameDialog *r = new RenameDialog(this);
    // Get function based on click position
    //r->setFunctionName(fcn_name);
    r->open();
}

void MainWindow::addOutput(const QString &msg)
{
    consoleWidget->addOutput(msg);
}

void MainWindow::addDebugOutput(const QString &msg)
{
    printf("debug output: %s\n", msg.toLocal8Bit().constData());
    consoleWidget->addDebugOutput(msg);
}

void MainWindow::on_actionNew_triggered()
{
    if (close())
        on_actionLoad_triggered();
}

void MainWindow::on_actionSave_triggered()
{
    saveProject();
}

void MainWindow::on_actionRun_Script_triggered()
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setViewMode(QFileDialog::Detail);
    dialog.setDirectory(QDir::home());

    QString fileName;
    fileName = dialog.getOpenFileName(this, tr("Select radare2 script"));
    if (!fileName.length()) //cancel was pressed
        return;

    qDebug() << "Meow: " + fileName;
    this->core->cmd(". " + fileName);
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
    if (ui->sideToolBar->isVisible())
    {
        ui->sideToolBar->hide();
    }
    else
    {
        ui->sideToolBar->show();
    }
}

void MainWindow::on_actionDashboard_triggered()
{
    if (this->dashboardDock->isVisible())
    {
        this->dashboardDock->close();
    }
    else
    {
        this->dashboardDock->show();
        this->dashboardDock->raise();
    }
}

void MainWindow::on_actionForward_triggered()
{
    this->core->cmd("s+");
    RVA offset = core->getOffset();
    this->addDebugOutput(QString::number(offset));
    core->seek(offset);
}

void MainWindow::toggleResponsive(bool maybe)
{
    this->responsive = maybe;
    // Save options in settings
    QSettings settings;
    settings.setValue("responsive", this->responsive);
}

void MainWindow::on_actionTabs_on_Top_triggered()
{
    this->on_actionTabs_triggered();
}

void MainWindow::on_actionReset_settings_triggered()
{
    QMessageBox::StandardButton ret =
        (QMessageBox::StandardButton)QMessageBox::question(this, APPNAME,
                tr("Do you really want to clear all settings?"),
                QMessageBox::Ok | QMessageBox::Cancel);
    if (ret == QMessageBox::Ok)
    {
        // Save options in settings
        QSettings settings;
        settings.clear();
    }
}

void MainWindow::on_actionQuit_triggered()
{
    close();
}

void MainWindow::refreshVisibleDockWidgets()
{
    // There seems to be no convenience function to check if a QDockWidget
    // is really visible or hidden in a tabbed dock. So:
    auto isDockVisible = [](const QDockWidget * const pWidget)
    {
        return pWidget != nullptr && !pWidget->visibleRegion().isEmpty();
    };

    for (auto w : dockWidgets)
    {
        if (isDockVisible(w))
        {
            w->refresh();
        }
    }
}

void MainWindow::on_actionRefresh_contents_triggered()
{
    refreshVisibleDockWidgets();
}

void MainWindow::on_actionAsmOptions_triggered()
{
    auto dialog = new AsmOptionsDialog(core, this);
    dialog->show();
}
