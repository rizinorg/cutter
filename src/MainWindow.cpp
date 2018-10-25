#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "common/Helpers.h"

#include <QApplication>
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
#include <QSvgRenderer>

#include "common/Highlighter.h"
#include "common/HexAsciiHighlighter.h"
#include "common/Helpers.h"
#include "common/SvgIconEngine.h"
#include "common/ProgressIndicator.h"
#include "common/TempConfig.h"

#include "dialogs/NewFileDialog.h"
#include "dialogs/InitialOptionsDialog.h"
#include "dialogs/SaveProjectDialog.h"
#include "dialogs/CommentsDialog.h"
#include "dialogs/AboutDialog.h"
#include "dialogs/RenameDialog.h"
#include "dialogs/preferences/PreferencesDialog.h"
#include "dialogs/OpenFileDialog.h"

#include "widgets/DisassemblerGraphView.h"
#include "widgets/GraphWidget.h"
#include "widgets/FunctionsWidget.h"
#include "widgets/SectionsWidget.h"
#include "widgets/SegmentsWidget.h"
#include "widgets/CommentsWidget.h"
#include "widgets/ImportsWidget.h"
#include "widgets/ExportsWidget.h"
#include "widgets/TypesWidget.h"
#include "widgets/SearchWidget.h"
#include "widgets/SymbolsWidget.h"
#include "widgets/StringsWidget.h"
#include "widgets/RelocsWidget.h"
#include "widgets/FlagsWidget.h"
#include "widgets/VisualNavbar.h"
#include "widgets/Dashboard.h"
#include "widgets/Sidebar.h"
#include "widgets/SdbDock.h"
#include "widgets/Omnibar.h"
#include "widgets/ConsoleWidget.h"
#include "widgets/EntrypointWidget.h"
#include "widgets/ClassesWidget.h"
#include "widgets/ResourcesWidget.h"
#include "widgets/VTablesWidget.h"
#include "widgets/JupyterWidget.h"
#include "widgets/HeadersWidget.h"
#include "widgets/ZignaturesWidget.h"
#include "widgets/DebugToolbar.h"
#include "widgets/MemoryMapWidget.h"
#include "widgets/BreakpointWidget.h"
#include "widgets/RegisterRefsWidget.h"

// Graphics
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

    // Do not issue a warning in release
    Q_UNUSED(ret)
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    core(Core()),
    ui(new Ui::MainWindow)
{
    panelLock = false;
    tabsOnTop = false;
    configuration = Config();
}

MainWindow::~MainWindow()
{
}

void MainWindow::initUI()
{
    ui->setupUi(this);

    registerCustomFonts();

    /*
    * Toolbar
    */

    // Sepparator between undo/redo and goto lineEdit
    QWidget *spacer3 = new QWidget();
    spacer3->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    spacer3->setMinimumSize(20, 20);
    spacer3->setMaximumWidth(100);
    ui->mainToolBar->addWidget(spacer3);

    DebugToolbar *debugToolbar = new DebugToolbar(this);
    ui->mainToolBar->addWidget(debugToolbar);
    // Debug menu
    // ui->menuDebug->addAction(debugToolbar->actionStart);
    ui->menuDebug->addAction(debugToolbar->actionStartEmul);
    // ui->menuDebug->addAction(debugToolbar->actionAttach);
    ui->menuDebug->addSeparator();
    ui->menuDebug->addAction(debugToolbar->actionStep);
    ui->menuDebug->addAction(debugToolbar->actionStepOver);
    ui->menuDebug->addAction(debugToolbar->actionStepOut);
    ui->menuDebug->addSeparator();
    ui->menuDebug->addAction(debugToolbar->actionContinue);
    ui->menuDebug->addAction(debugToolbar->actionContinueUntilCall);
    ui->menuDebug->addAction(debugToolbar->actionContinueUntilSyscall);

    // Sepparator between undo/redo and goto lineEdit
    QWidget *spacer4 = new QWidget();
    spacer4->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    spacer4->setMinimumSize(20, 20);
    spacer4->setMaximumWidth(100);
    ui->mainToolBar->addWidget(spacer4);

    // Omnibar LineEdit
    this->omnibar = new Omnibar(this);
    ui->mainToolBar->addWidget(this->omnibar);

    // Add special separators to the toolbar that expand to separate groups of elements
    QWidget *spacer2 = new QWidget();
    spacer2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    spacer2->setMinimumSize(10, 10);
    spacer2->setMaximumWidth(300);
    ui->mainToolBar->addWidget(spacer2);

    // Separator between back/forward and undo/redo buttons
    QWidget *spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    spacer->setMinimumSize(20, 20);
    ui->mainToolBar->addWidget(spacer);

    tasksProgressIndicator = new ProgressIndicator();
    ui->mainToolBar->addWidget(tasksProgressIndicator);

    QWidget *spacerEnd = new QWidget();
    spacerEnd->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    spacerEnd->setMinimumSize(4, 0);
    spacerEnd->setMaximumWidth(4);
    ui->mainToolBar->addWidget(spacerEnd);

    // Visual navigation tool bar
    this->visualNavbar = new VisualNavbar(this);
    this->visualNavbar->setMovable(false);
    addToolBarBreak(Qt::TopToolBarArea);
    addToolBar(visualNavbar);
    QObject::connect(configuration, &Configuration::colorsUpdated, [this]() {
        this->visualNavbar->updateGraphicsScene();
    });

    /*
     * Dock Widgets
     */
    dockWidgets.reserve(20);

    disassemblyDock = new DisassemblyWidget(this, ui->actionDisassembly);
    sidebarDock = new SidebarWidget(this, ui->actionSidebar);
    hexdumpDock = new HexdumpWidget(this, ui->actionHexdump);
    pseudocodeDock = new PseudocodeWidget(this, ui->actionPseudocode);
    consoleDock = new ConsoleWidget(this, ui->actionConsole);

    // Add graph view as dockable
    graphDock = new GraphWidget(this, ui->actionGraph);

    sectionsDock = new SectionsWidget(this, ui->actionSections);
    segmentsDock = new SegmentsWidget(this, ui->actionSegments);
    entrypointDock = new EntrypointWidget(this, ui->actionEntrypoints);
    functionsDock = new FunctionsWidget(this, ui->actionFunctions);
    importsDock = new ImportsWidget(this, ui->actionImports);
    exportsDock = new ExportsWidget(this, ui->actionExports);
    headersDock = new HeadersWidget(this, ui->actionHeaders);
    zignaturesDock = new ZignaturesWidget(this, ui->actionZignatures);
    typesDock = new TypesWidget(this, ui->actionTypes);
    searchDock = new SearchWidget(this, ui->actionSearch);
    symbolsDock = new SymbolsWidget(this, ui->actionSymbols);
    relocsDock = new RelocsWidget(this, ui->actionRelocs);
    commentsDock = new CommentsWidget(this, ui->actionComments);
    stringsDock = new StringsWidget(this, ui->actionStrings);
    flagsDock = new FlagsWidget(this, ui->actionFlags);
    stackDock = new StackWidget(this, ui->actionStack);
    backtraceDock = new BacktraceWidget(this, ui->actionBacktrace);
    registersDock = new RegistersWidget(this, ui->actionRegisters);
    memoryMapDock = new MemoryMapWidget(this, ui->actionMemoryMap);
    breakpointDock = new BreakpointWidget(this, ui->actionBreakpoint);
    registerRefsDock = new RegisterRefsWidget(this, ui->actionRegisterRefs);
#ifdef CUTTER_ENABLE_JUPYTER
    jupyterDock = new JupyterWidget(this, ui->actionJupyter);
#else
    ui->actionJupyter->setEnabled(false);
    ui->actionJupyter->setVisible(false);
#endif
    dashboardDock = new Dashboard(this, ui->actionDashboard);
    sdbDock = new SdbDock(this, ui->actionSDBBrowser);
    classesDock = new ClassesWidget(this, ui->actionClasses);
    resourcesDock = new ResourcesWidget(this, ui->actionResources);
    vTablesDock = new VTablesWidget(this, ui->actionVTables);


    // Set up dock widgets default layout
    resetToDefaultLayout();
    enableDebugWidgetsMenu(false);

    // Restore saved settings
    this->readSettings();
    // TODO: Allow the user to select this option visually in the GUI settings
    // Adjust the DockWidget areas
    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    //setCorner( Qt::TopRightCorner, Qt::RightDockWidgetArea );
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    //setCorner( Qt::BottomRightCorner, Qt::RightDockWidgetArea );


    /*
     *  Some global shortcuts
     */
    // Period goes to command entry
    QShortcut *cmd_shortcut = new QShortcut(QKeySequence(Qt::Key_Period), this);
    connect(cmd_shortcut, SIGNAL(activated()), consoleDock, SLOT(focusInputLineEdit()));

    // G and S goes to goto entry
    QShortcut *goto_shortcut = new QShortcut(QKeySequence(Qt::Key_G), this);
    connect(goto_shortcut, SIGNAL(activated()), this->omnibar, SLOT(setFocus()));
    QShortcut *seek_shortcut = new QShortcut(QKeySequence(Qt::Key_S), this);
    connect(seek_shortcut, SIGNAL(activated()), this->omnibar, SLOT(setFocus()));

    QShortcut *refresh_shortcut = new QShortcut(QKeySequence(QKeySequence::Refresh), this);
    connect(refresh_shortcut, SIGNAL(activated()), this, SLOT(refreshAll()));

    connect(core, SIGNAL(projectSaved(const QString &)), this, SLOT(projectSaved(const QString &)));

    connect(core, &CutterCore::changeDebugView, this, &MainWindow::changeDebugView);
    connect(core, &CutterCore::changeDefinedView, this, &MainWindow::changeDefinedView);

    connect(core, SIGNAL(newMessage(const QString &)),
            this->consoleDock, SLOT(addOutput(const QString &)));
    connect(core, SIGNAL(newDebugMessage(const QString &)),
            this->consoleDock, SLOT(addDebugOutput(const QString &)));

    updateTasksIndicator();
    connect(core->getAsyncTaskManager(), &AsyncTaskManager::tasksChanged, this,
            &MainWindow::updateTasksIndicator);

    /* Load plugins */
    QList<CutterPlugin *> plugins = core->getCutterPlugins();
    for (auto plugin : plugins) {
        CutterDockWidget *pluginDock = plugin->setupInterface(this);
        tabifyDockWidget(dashboardDock, pluginDock);
    }
}

void MainWindow::updateTasksIndicator()
{
    bool running = core->getAsyncTaskManager()->getTasksRunning();
    tasksProgressIndicator->setProgressIndicatorVisible(running);
}

void MainWindow::on_actionExtraGraph_triggered()
{
    QDockWidget *extraDock = new GraphWidget(this, 0);
    addExtraWidget(extraDock);
}

void MainWindow::on_actionExtraHexdump_triggered()
{
    QDockWidget *extraDock = new HexdumpWidget(this, 0);
    addExtraWidget(extraDock);
}

void MainWindow::on_actionExtraDisassembly_triggered()
{
    QDockWidget *extraDock = new DisassemblyWidget(this, 0);
    addExtraWidget(extraDock);
}

void MainWindow::addExtraWidget(QDockWidget *extraDock)
{
    addDockWidget(Qt::TopDockWidgetArea, extraDock);
    auto restoreExtraDock = qhelpers::forceWidth(extraDock->widget(), 600);
    qApp->processEvents();
    restoreExtraDock.restoreWidth(extraDock->widget());
}

void MainWindow::openNewFile(InitialOptions options, bool skipOptionsDialog)
{
    setFilename(options.filename);

    /* Prompt to load filename.r2 script */
    if (options.script.isEmpty()) {
        QString script = QString("%1.r2").arg(this->filename);
        if (r_file_exists(script.toStdString().data())) {
            QMessageBox mb;
            mb.setWindowTitle(tr("Script loading"));
            mb.setText(tr("Do you want to load the '%1' script?").arg(script));
            mb.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            if (mb.exec() == QMessageBox::Yes) {
                options.script = script;
            }
        }
    }

    /* Show analysis options dialog */
    displayInitialOptionsDialog(options, skipOptionsDialog);
}

void MainWindow::openNewFileFailed()
{
    displayNewFileDialog();
    QMessageBox mb(this);
    mb.setIcon(QMessageBox::Critical);
    mb.setStandardButtons(QMessageBox::Ok);
    mb.setWindowTitle(tr("Cannot open file!"));
    mb.setText(
        tr("Could not open the file! Make sure the file exists and that you have the correct permissions."));
    mb.exec();
}

void MainWindow::displayNewFileDialog()
{
    NewFileDialog *n = new NewFileDialog();
    newFileDialog = n;
    n->setAttribute(Qt::WA_DeleteOnClose);
    n->show();
}

void MainWindow::closeNewFileDialog()
{
    if (newFileDialog) {
        newFileDialog->close();
    }
    newFileDialog = nullptr;
}

void MainWindow::displayInitialOptionsDialog(const InitialOptions &options, bool skipOptionsDialog)
{
    auto o = new InitialOptionsDialog(this);
    o->setAttribute(Qt::WA_DeleteOnClose);
    o->loadOptions(options);

    if (skipOptionsDialog) {
        o->setupAndStartAnalysis();
    } else {
        o->show();
    }
}

void MainWindow::openProject(const QString &project_name)
{
    QString filename = core->cmd("Pi " + project_name);
    setFilename(filename.trimmed());

    core->openProject(project_name);

    initUI();
    finalizeOpen();
}

void MainWindow::finalizeOpen()
{
    core->getOpcodes();

    // Override any incorrect setting saved in the project
    core->setSettings();

    core->updateSeek();
    core->message(tr(" > Populating UI"));
    refreshAll();

    core->message(tr(" > Finished, happy reversing :)"));
    // Add fortune message
    core->message("\n" + core->cmd("fo"));
    showMaximized();
}

bool MainWindow::saveProject(bool quit)
{
    QString projectName = core->getConfig("prj.name");
    if (projectName.isEmpty()) {
        return saveProjectAs(quit);
    } else {
        core->saveProject(projectName);
        return true;
    }
}

bool MainWindow::saveProjectAs(bool quit)
{
    SaveProjectDialog dialog(quit, this);
    return SaveProjectDialog::Rejected != dialog.exec();
}

void MainWindow::refreshOmniBar(const QStringList &flags)
{
    omnibar->refresh(flags);
}

void MainWindow::setFilename(const QString &fn)
{
    // Add file name to window title
    this->filename = fn;
    this->setWindowTitle(APPNAME" – " + fn);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QMessageBox::StandardButton ret = QMessageBox::question(this, APPNAME,
                                                            tr("Do you really want to exit?\nSave your project before closing!"),
                                                            (QMessageBox::StandardButtons)(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel));
    if (ret == QMessageBox::Cancel) {
        event->ignore();
        return;
    }

    if (ret == QMessageBox::Save && !saveProject(true)) {
        event->ignore();
        return;
    }

    if (!core->currentlyDebugging) {
        saveSettings();
    } else {
        core->stopDebug();
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::readSettings()
{
    QSettings settings;
    QByteArray geo = settings.value("geometry", QByteArray()).toByteArray();
    restoreGeometry(geo);
    QByteArray state = settings.value("state", QByteArray()).toByteArray();
    restoreState(state);
    this->responsive = settings.value("responsive").toBool();
    panelLock = settings.value("panelLock").toBool();
    setPanelLock();
    tabsOnTop = settings.value("tabsOnTop").toBool();
    setTabLocation();
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(400, 400)).toSize();
    resize(size);
    move(pos);
    updateDockActionsChecked();
}

void MainWindow::saveSettings()
{
    QSettings settings;
    settings.setValue("geometry", saveGeometry());
    settings.setValue("size", size());
    settings.setValue("pos", pos());
    settings.setValue("state", saveState());
    settings.setValue("panelLock", panelLock);
    settings.setValue("tabsOnTop", tabsOnTop);
}

void MainWindow::readDebugSettings()
{
    QSettings settings;
    QByteArray geo = settings.value("debug.geometry", QByteArray()).toByteArray();
    restoreGeometry(geo);
    QByteArray state = settings.value("debug.state", QByteArray()).toByteArray();
    restoreState(state);
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(400, 400)).toSize();
    resize(size);
    move(pos);
    updateDockActionsChecked();
}

void MainWindow::saveDebugSettings()
{
    QSettings settings;
    settings.setValue("debug.geometry", saveGeometry());
    settings.setValue("debug.state", saveState());
    settings.setValue("size", size());
    settings.setValue("pos", pos());
}

void MainWindow::setPanelLock()
{
    if (panelLock) {
        for (QDockWidget *dockWidget : findChildren<QDockWidget *>()) {
            dockWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
        }

        ui->actionLock->setChecked(false);
    } else {
        for (QDockWidget *dockWidget : findChildren<QDockWidget *>()) {
            dockWidget->setFeatures(QDockWidget::AllDockWidgetFeatures);
        }

        ui->actionLock->setChecked(true);
    }
}

void MainWindow::setTabLocation()
{
    if (tabsOnTop) {
        this->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);
        ui->actionTabs_on_Top->setChecked(true);
    } else {
        this->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::South);
        ui->actionTabs_on_Top->setChecked(false);
    }
}

void MainWindow::refreshAll()
{
    core->triggerRefreshAll();
}

void MainWindow::lockUnlock_Docks(bool what)
{
    if (what) {
        for (QDockWidget *dockWidget : findChildren<QDockWidget *>()) {
            dockWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
        }
    } else {
        for (QDockWidget *dockWidget : findChildren<QDockWidget *>()) {
            dockWidget->setFeatures(QDockWidget::AllDockWidgetFeatures);
        }
    }

}

void MainWindow::restoreDocks()
{
    // In the upper half the functions are the first widget
    addDockWidget(Qt::TopDockWidgetArea, functionsDock);

    // Function | Dashboard | Sidebar
    splitDockWidget(functionsDock, dashboardDock, Qt::Horizontal);
    splitDockWidget(dashboardDock, sidebarDock, Qt::Horizontal);

    // In the lower half the console is the first widget
    addDockWidget(Qt::BottomDockWidgetArea, consoleDock);

    // Console | Sections
    splitDockWidget(consoleDock, sectionsDock, Qt::Horizontal);
    splitDockWidget(consoleDock, segmentsDock, Qt::Horizontal);

    // Tabs for center (must be applied after splitDockWidget())
    tabifyDockWidget(sectionsDock, commentsDock);
    tabifyDockWidget(segmentsDock, commentsDock);
    tabifyDockWidget(dashboardDock, disassemblyDock);
    tabifyDockWidget(dashboardDock, graphDock);
    tabifyDockWidget(dashboardDock, hexdumpDock);
    tabifyDockWidget(dashboardDock, pseudocodeDock);
    tabifyDockWidget(dashboardDock, entrypointDock);
    tabifyDockWidget(dashboardDock, flagsDock);
    tabifyDockWidget(dashboardDock, stringsDock);
    tabifyDockWidget(dashboardDock, relocsDock);
    tabifyDockWidget(dashboardDock, importsDock);
    tabifyDockWidget(dashboardDock, exportsDock);
    tabifyDockWidget(dashboardDock, typesDock);
    tabifyDockWidget(dashboardDock, searchDock);
    tabifyDockWidget(dashboardDock, headersDock);
    tabifyDockWidget(dashboardDock, zignaturesDock);
    tabifyDockWidget(dashboardDock, symbolsDock);
    tabifyDockWidget(dashboardDock, classesDock);
    tabifyDockWidget(dashboardDock, resourcesDock);
    tabifyDockWidget(dashboardDock, vTablesDock);

    // Add Stack, Registers and Backtrace vertically stacked
    splitDockWidget(sidebarDock, stackDock, Qt::Horizontal);
    splitDockWidget(stackDock, registersDock, Qt::Vertical);
    tabifyDockWidget(stackDock, backtraceDock);
    // MemoryMap/Breakpoint/RegRefs widget goes in the center tabs
    tabifyDockWidget(dashboardDock, memoryMapDock);
    tabifyDockWidget(dashboardDock, breakpointDock);
    tabifyDockWidget(dashboardDock, registerRefsDock);
#ifdef CUTTER_ENABLE_JUPYTER
    tabifyDockWidget(dashboardDock, jupyterDock);
#endif

    updateDockActionsChecked();
}


void MainWindow::hideAllDocks()
{
    for (auto w : dockWidgets) {
        removeDockWidget(w);
    }

    updateDockActionsChecked();
}

void MainWindow::updateDockActionsChecked()
{
    for (auto i = dockWidgetActions.constBegin(); i != dockWidgetActions.constEnd(); i++) {
        i.key()->setChecked(!i.value()->isHidden());
    }
}

void MainWindow::updateDockActionChecked(QAction *action)
{
    action->setChecked(!dockWidgetActions[action]->isHidden());
}

void MainWindow::showZenDocks()
{
    const QList<QDockWidget *> zenDocks = { functionsDock,
                                            dashboardDock,
                                            stringsDock,
                                            graphDock,
                                            disassemblyDock,
                                            hexdumpDock,
                                            searchDock,
                                            importsDock,
#ifdef CUTTER_ENABLE_JUPYTER
                                            jupyterDock
#endif
                                          };
    for (auto w : dockWidgets) {
        if (zenDocks.contains(w)) {
            w->show();
        }
    }
    updateDockActionsChecked();
}

void MainWindow::showDebugDocks()
{
    const QList<QDockWidget *> debugDocks = { functionsDock,
                                              stringsDock,
                                              graphDock,
                                              disassemblyDock,
                                              hexdumpDock,
                                              searchDock,
                                              stackDock,
                                              registersDock,
                                              backtraceDock,
                                              memoryMapDock,
                                              breakpointDock
                                            };
    for (auto w : dockWidgets) {
        if (debugDocks.contains(w)) {
            w->show();
        }
    }
    updateDockActionsChecked();
}

void MainWindow::enableDebugWidgetsMenu(bool enable)
{
    ui->menuAddDebugWidgets->setEnabled(enable);
}

void MainWindow::resetToDefaultLayout()
{
    hideAllDocks();
    restoreDocks();
    showZenDocks();
    dashboardDock->raise();

    // Ugly workaround to set the default widths of functions docks
    // if anyone finds a way to do this cleaner that also works, feel free to change it!
    auto restoreFunctionDock = qhelpers::forceWidth(functionsDock->widget(), 200);
    qApp->processEvents();
    restoreFunctionDock.restoreWidth(functionsDock->widget());

    core->setMemoryWidgetPriority(CutterCore::MemoryWidgetType::Disassembly);
}

void MainWindow::resetToDebugLayout()
{
    CutterCore::MemoryWidgetType memType = core->getMemoryWidgetPriority();
    bool isMaxim = isMaximized();
    hideAllDocks();
    restoreDocks();
    showDebugDocks();
    readDebugSettings();
    core->raisePrioritizedMemoryWidget(memType);
    if (isMaxim) {
        showMaximized();
    } else {
        showNormal();
    }
}

void MainWindow::on_actionLock_triggered()
{
    panelLock = !panelLock;
    setPanelLock();
}

void MainWindow::on_actionLockUnlock_triggered()
{
    if (ui->actionLockUnlock->isChecked()) {
        for (QDockWidget *dockWidget : findChildren<QDockWidget *>()) {
            dockWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
        }
        ui->actionLockUnlock->setIcon(QIcon(":/lock"));
    } else {
        for (QDockWidget *dockWidget : findChildren<QDockWidget *>()) {
            dockWidget->setFeatures(QDockWidget::AllDockWidgetFeatures);
        }
        ui->actionLockUnlock->setIcon(QIcon(":/unlock"));
    }
}

void MainWindow::on_actionFunctionsRename_triggered()
{
    RenameDialog *r = new RenameDialog(this);
    // Get function based on click position
    //r->setFunctionName(fcn_name);
    r->open();
}

void MainWindow::on_actionDefault_triggered()
{
    resetToDefaultLayout();
}

/**
 * @brief MainWindow::on_actionNew_triggered
 * Open a new Cutter session.
 */
void MainWindow::on_actionNew_triggered()
{
    // Create a new Cutter process
    QProcess process(this);
    process.setEnvironment(QProcess::systemEnvironment());
    process.startDetached(qApp->applicationFilePath());
}

void MainWindow::on_actionSave_triggered()
{
    saveProject();
}

void MainWindow::on_actionSaveAs_triggered()
{
    saveProjectAs();
}

void MainWindow::on_actionRun_Script_triggered()
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setViewMode(QFileDialog::Detail);
    dialog.setDirectory(QDir::home());

    QString fileName;
    fileName = dialog.getOpenFileName(this, tr("Select radare2 script"));
    if (!fileName.length()) // Cancel was pressed
        return;
    core->loadScript(fileName);
}

/**
 * @brief MainWindow::on_actionOpen_triggered
 * Open a file as in "load (add) a file in current session".
 */
void MainWindow::on_actionOpen_triggered()
{
    OpenFileDialog dialog(this);
    dialog.exec();
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
    if (ret == QMessageBox::Ok) {
        Config()->resetAll();
    }
}

void MainWindow::on_actionQuit_triggered()
{
    close();
}

void MainWindow::on_actionBackward_triggered()
{
    core->seekPrev();
}

void MainWindow::on_actionForward_triggered()
{
    core->seekNext();
}

void MainWindow::on_actionUndoSeek_triggered()
{
    core->seekPrev();
}

void MainWindow::on_actionRedoSeek_triggered()
{
    core->seekNext();
}

void MainWindow::on_actionDisasAdd_comment_triggered()
{
    CommentsDialog *c = new CommentsDialog(this);
    c->exec();
    delete c;
}

void MainWindow::on_actionRefresh_contents_triggered()
{
    refreshAll();
}

void MainWindow::on_actionPreferences_triggered()
{
    auto dialog = new PreferencesDialog(this);
    dialog->show();
}

void MainWindow::on_actionTabs_triggered()
{
    tabsOnTop = !tabsOnTop;
    setTabLocation();
}

void MainWindow::on_actionAbout_triggered()
{
    AboutDialog *a = new AboutDialog(this);
    a->open();
}

void MainWindow::on_actionRefresh_Panels_triggered()
{
    this->refreshAll();
}

void MainWindow::on_actionAnalyze_triggered()
{
    // TODO: implement this, but do NOT open InitialOptionsDialog!!
}

void MainWindow::on_actionImportPDB_triggered()
{
    QFileDialog dialog(this);
    dialog.setWindowTitle(tr("Select PDB file"));
    dialog.setNameFilters({ tr("PDB file (*.pdb)"), tr("All files (*)") });

    if (!dialog.exec()) {
        return;
    }

    QString pdbFile = dialog.selectedFiles().first();

    if (!pdbFile.isEmpty()) {
        core->loadPDB(pdbFile);
        core->message(tr("%1 loaded.").arg(pdbFile));
    }
}

void MainWindow::on_actionExport_as_code_triggered()
{
    QStringList filters;
    QMap<QString, QString> cmdMap;

    filters << tr("C uin8_t array (*.c)");
    cmdMap[filters.last()] = "pc";
    filters << tr("C uin16_t array (*.c)");
    cmdMap[filters.last()] = "pch";
    filters << tr("C uin32_t array (*.c)");
    cmdMap[filters.last()] = "pcw";
    filters << tr("C uin64_t array (*.c)");
    cmdMap[filters.last()] = "pcd";
    filters << tr("C string (*.c)");
    cmdMap[filters.last()] = "pcs";
    filters << tr("Shell-script that reconstructs the bin (*.sh)");
    cmdMap[filters.last()] = "pcS";
    filters << tr("JSON array (*.json)");
    cmdMap[filters.last()] = "pcj";
    filters << tr("JavaScript array (*.js)");
    cmdMap[filters.last()] = "pcJ";
    filters << tr("Python array (*.py)");
    cmdMap[filters.last()] = "pcp";
    filters << tr("Print 'wx' r2 commands (*.r2)");
    cmdMap[filters.last()] = "pc*";
    filters << tr("GAS .byte blob (*.asm, *.s)");
    cmdMap[filters.last()] = "pca";
    filters << tr(".bytes with instructions in comments (*.txt)");
    cmdMap[filters.last()] = "pcA";

    QFileDialog dialog(this, tr("Export as code"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilters(filters);
    dialog.selectFile("dump");
    dialog.setDefaultSuffix("c");
    if (!dialog.exec())
        return;

    QFile file(dialog.selectedFiles()[0]);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Can't open file";
        return;
    }
    TempConfig tempConfig;
    tempConfig.set("io.va", false);
    QTextStream fileOut(&file);
    QString &cmd = cmdMap[dialog.selectedNameFilter()];
    fileOut << Core()->cmd(cmd + " $s @ 0");
}


void MainWindow::projectSaved(const QString &name)
{
    core->message(tr("Project saved: ") + name);
}

void MainWindow::changeDebugView()
{
    saveSettings();
    resetToDebugLayout();
    enableDebugWidgetsMenu(true);
}

void MainWindow::changeDefinedView()
{
    saveDebugSettings();
    CutterCore::MemoryWidgetType memType = core->getMemoryWidgetPriority();
    hideAllDocks();
    restoreDocks();
    readSettings();
    enableDebugWidgetsMenu(false);
    core->raisePrioritizedMemoryWidget(memType);
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    switch (event->button()) {
    case Qt::BackButton:
        core->seekPrev();
        break;
    case Qt::ForwardButton:
        core->seekNext();
        break;
    default:
        break;
    }
}

bool MainWindow::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::ForwardButton || mouseEvent->button() == Qt::BackButton) {
            mousePressEvent(mouseEvent);
            return true;
        }
    }
    return false;
}

void MainWindow::addToDockWidgetList(QDockWidget *dockWidget)
{
    this->dockWidgets.push_back(dockWidget);
}

void MainWindow::addDockWidgetAction(QDockWidget *dockWidget, QAction *action)
{
    this->dockWidgetActions[action] = dockWidget;
}
