#include "core/MainWindow.h"
#include "ui_MainWindow.h"

// Common Headers
#include "common/BugReporting.h"
#include "common/Highlighter.h"
#include "common/HexAsciiHighlighter.h"
#include "common/Helpers.h"
#include "common/SvgIconEngine.h"
#include "common/ProgressIndicator.h"
#include "common/TempConfig.h"
#include "common/RunScriptTask.h"
#include "common/PythonManager.h"
#include "plugins/PluginManager.h"
#include "CutterConfig.h"

// Dialogs
#include "dialogs/WelcomeDialog.h"
#include "dialogs/NewFileDialog.h"
#include "dialogs/InitialOptionsDialog.h"
#include "dialogs/SaveProjectDialog.h"
#include "dialogs/CommentsDialog.h"
#include "dialogs/AboutDialog.h"
#include "dialogs/RenameDialog.h"
#include "dialogs/preferences/PreferencesDialog.h"
#include "dialogs/OpenFileDialog.h"
#include "dialogs/AsyncTaskDialog.h"

// Widgets Headers
#include "widgets/DisassemblerGraphView.h"
#include "widgets/GraphView.h"
#include "widgets/GraphWidget.h"
#include "widgets/OverviewWidget.h"
#include "widgets/OverviewView.h"
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
#include "widgets/SdbWidget.h"
#include "widgets/Omnibar.h"
#include "widgets/ConsoleWidget.h"
#include "widgets/EntrypointWidget.h"
#include "widgets/ClassesWidget.h"
#include "widgets/ResourcesWidget.h"
#include "widgets/VTablesWidget.h"
#include "widgets/HeadersWidget.h"
#include "widgets/ZignaturesWidget.h"
#include "widgets/DebugActions.h"
#include "widgets/MemoryMapWidget.h"
#include "widgets/BreakpointWidget.h"
#include "widgets/RegisterRefsWidget.h"
#include "widgets/DisassemblyWidget.h"
#include "widgets/StackWidget.h"
#include "widgets/RegistersWidget.h"
#include "widgets/BacktraceWidget.h"
#include "widgets/HexdumpWidget.h"
#include "widgets/PseudocodeWidget.h"
#include "widgets/HexWidget.h"

// Qt Headers
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
#include <QSysInfo>
#include <QJsonObject>

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

// Graphics
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include <QGraphicsView>

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

    initToolBar();
    initDocks();

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

    connect(core, SIGNAL(projectSaved(bool, const QString &)), this, SLOT(projectSaved(bool,
                                                                                       const QString &)));

    connect(core, &CutterCore::changeDebugView, this, &MainWindow::changeDebugView);
    connect(core, &CutterCore::changeDefinedView, this, &MainWindow::changeDefinedView);

    connect(core, SIGNAL(newMessage(const QString &)),
            this->consoleDock, SLOT(addOutput(const QString &)));
    connect(core, SIGNAL(newDebugMessage(const QString &)),
            this->consoleDock, SLOT(addDebugOutput(const QString &)));

    updateTasksIndicator();
    connect(core->getAsyncTaskManager(), &AsyncTaskManager::tasksChanged, this,
            &MainWindow::updateTasksIndicator);

    //Undo and redo seek
    ui->actionBackward->setShortcut(QKeySequence::Back);
    ui->actionForward->setShortcut(QKeySequence::Forward);
    
    /* Setup plugins interfaces */
    for (auto plugin : Plugins()->getPlugins()) {
        plugin->setupInterface(this);
    }

    initLayout();
}

void MainWindow::initToolBar()
{
    chooseThemeIcons();
    
    // Sepparator between undo/redo and goto lineEdit
    QWidget *spacer3 = new QWidget();
    spacer3->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    spacer3->setStyleSheet("background-color: rgba(0,0,0,0)");
    spacer3->setMinimumSize(20, 20);
    spacer3->setMaximumWidth(100);
    ui->mainToolBar->addWidget(spacer3);

    DebugActions *debugActions = new DebugActions(ui->mainToolBar, this);
    // Debug menu
    ui->menuDebug->addAction(debugActions->actionStartEmul);
    ui->menuDebug->addSeparator();
    ui->menuDebug->addAction(debugActions->actionStep);
    ui->menuDebug->addAction(debugActions->actionStepOver);
    ui->menuDebug->addAction(debugActions->actionStepOut);
    ui->menuDebug->addSeparator();
    ui->menuDebug->addAction(debugActions->actionContinue);
    ui->menuDebug->addAction(debugActions->actionContinueUntilCall);
    ui->menuDebug->addAction(debugActions->actionContinueUntilSyscall);

    // Sepparator between undo/redo and goto lineEdit
    QWidget *spacer4 = new QWidget();
    spacer4->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    spacer4->setStyleSheet("background-color: rgba(0,0,0,0)");
    spacer4->setMinimumSize(10, 10);
    spacer4->setMaximumWidth(10);
    ui->mainToolBar->addWidget(spacer4);

    // Omnibar LineEdit
    this->omnibar = new Omnibar(this);
    ui->mainToolBar->addWidget(this->omnibar);

    // Add special separators to the toolbar that expand to separate groups of elements
    QWidget *spacer2 = new QWidget();
    spacer2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    spacer2->setStyleSheet("background-color: rgba(0,0,0,0)");
    spacer2->setMinimumSize(10, 10);
    spacer2->setMaximumWidth(300);
    ui->mainToolBar->addWidget(spacer2);

    // Separator between back/forward and undo/redo buttons
    QWidget *spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    spacer->setStyleSheet("background-color: rgba(0,0,0,0)");
    spacer->setMinimumSize(20, 20);
    ui->mainToolBar->addWidget(spacer);

    tasksProgressIndicator = new ProgressIndicator();
    tasksProgressIndicator->setStyleSheet("background-color: rgba(0,0,0,0)");
    ui->mainToolBar->addWidget(tasksProgressIndicator);

    QWidget *spacerEnd = new QWidget();
    spacerEnd->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    spacerEnd->setStyleSheet("background-color: rgba(0,0,0,0)");
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
    QObject::connect(configuration, &Configuration::interfaceThemeChanged, this, &MainWindow::chooseThemeIcons);
}

void MainWindow::initDocks()
{
    dockWidgets.reserve(20);
    disassemblyDock = new DisassemblyWidget(this, ui->actionDisassembly);
    hexdumpDock = new HexdumpWidget(this, ui->actionHexdump);
    pseudocodeDock = new PseudocodeWidget(this, ui->actionPseudocode);
    consoleDock = new ConsoleWidget(this, ui->actionConsole);

    overviewDock = new OverviewWidget(this, ui->actionOverview);
    overviewDock->hide();
    connect(overviewDock, &OverviewWidget::isAvailableChanged, this, [this](bool isAvailable) {
        ui->actionOverview->setEnabled(isAvailable);
    });
    ui->actionOverview->setEnabled(overviewDock->getIsAvailable());
    connect(ui->actionOverview, &QAction::toggled, [this](bool checked) {
        if (checked) {
            overviewDock->show();
        } else {
            overviewDock->hide();
        }
    });

    ui->actionOverview->setChecked(overviewDock->getUserOpened());
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
    dashboardDock = new Dashboard(this, ui->actionDashboard);
    sdbDock = new SdbWidget(this, ui->actionSDBBrowser);
    classesDock = new ClassesWidget(this, ui->actionClasses);
    resourcesDock = new ResourcesWidget(this, ui->actionResources);
    vTablesDock = new VTablesWidget(this, ui->actionVTables);
}

void MainWindow::initLayout()
{
    // Set up dock widgets default layout
    enableDebugWidgetsMenu(false);
    // Restore saved settings
    readSettingsOrDefault();
    // TODO: Allow the user to select this option visually in the GUI settings
    // Adjust the DockWidget areas
    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
}

void MainWindow::toggleOverview(bool visibility, GraphWidget *targetGraph)
{
    if (!overviewDock) {
        return;
    }
    if (visibility) {
        overviewDock->setTargetGraphWidget(targetGraph);
    }
}

void MainWindow::updateTasksIndicator()
{
    bool running = core->getAsyncTaskManager()->getTasksRunning();
    tasksProgressIndicator->setProgressIndicatorVisible(running);
}

void MainWindow::on_actionExtraGraph_triggered()
{
    auto *extraDock = new GraphWidget(this, nullptr);
    addExtraWidget(extraDock);
}

void MainWindow::on_actionExtraHexdump_triggered()
{
    auto *extraDock = new HexdumpWidget(this, nullptr);
    addExtraWidget(extraDock);
}

void MainWindow::on_actionExtraDisassembly_triggered()
{
    auto *extraDock = new DisassemblyWidget(this, nullptr);
    addExtraWidget(extraDock);
}

void MainWindow::addExtraWidget(CutterDockWidget *extraDock)
{
    extraDock->setTransient(true);
    addDockWidget(Qt::TopDockWidgetArea, extraDock);
    auto restoreExtraDock = qhelpers::forceWidth(extraDock->widget(), 600);
    qApp->processEvents();
    restoreExtraDock.restoreWidth(extraDock->widget());
}

/**
 * @brief Getter for MainWindow's different menus
 * @param type The type which represents the desired menu
 * @return The requested menu or nullptr if "type" is invalid
**/
QMenu *MainWindow::getMenuByType(MenuType type)
{
    switch (type) {
    case MenuType::File:
        return ui->menuFile;
    case MenuType::Edit:
        return ui->menuEdit;
    case MenuType::View:
        return ui->menuView;
    case MenuType::Windows:
        return ui->menuWindows;
    case MenuType::Debug:
        return ui->menuDebug;
    case MenuType::Help:
        return ui->menuHelp;
    case MenuType::Plugins:
        return ui->menuPlugins;
    default:
        return nullptr;
    }
}

void MainWindow::addPluginDockWidget(QDockWidget *dockWidget, QAction *action)
{
    addDockWidget(Qt::TopDockWidgetArea, dockWidget);
    addDockWidgetAction(dockWidget, action);
    ui->menuPlugins->addAction(action);
    addDockWidget(Qt::DockWidgetArea::TopDockWidgetArea, dockWidget);
    updateDockActionChecked(action);
}

void MainWindow::addMenuFileAction(QAction *action)
{
    ui->menuFile->addAction(action);
}

void MainWindow::openNewFile(InitialOptions &options, bool skipOptionsDialog)
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

/**
 * @brief displays the WelocmeDialog
 *
 * Upon first execution of Cutter, the WelcomeDialog would be showed to the user.
 * The Welcome dialog would be showed after a reset of Cutter's preferences by the user.
 */

void MainWindow::displayWelcomeDialog()
{
    WelcomeDialog w;
    w.exec();
}

void MainWindow::displayNewFileDialog()
{
    NewFileDialog *n = new NewFileDialog(this);
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
    core->updateSeek();
    refreshAll();

    // Add fortune message
    core->message("\n" + core->cmd("fo"));
    showMaximized();



    // Set focus to disasm or graph widget

    // Use for loop to cover cases when main disasm/graph
    // (MainWindow::disassemblyDock and MainWindow::graphDock)
    // widgets are invisible but extra ones are visible

    // Graph with function in it has focus priority over DisasmWidget
    // if there are both graph and disasm.
    // Otherwise Disasm has focus priority over Graph

    // If there are no graph/disasm widgets focus on MainWindow

    setFocus();
    const QString disasmWidgetClassName = disassemblyDock->metaObject()->className();
    const QString graphWidgetClassName = graphDock->metaObject()->className();
    bool graphContainsFunc = false;
    for (auto dockWidget : dockWidgets) {
        const QString className = dockWidget->metaObject()->className();
        if (className == graphWidgetClassName && !dockWidget->visibleRegion().isNull()) {
            graphContainsFunc = !qobject_cast<GraphWidget*>(dockWidget)->getGraphView()->getBlocks().empty();
            if (graphContainsFunc) {
                dockWidget->widget()->setFocus();
                break;
            }
        }
        if (className == disasmWidgetClassName && !dockWidget->visibleRegion().isNull()) {
            if (!graphContainsFunc) {
                auto disasm = qobject_cast<DisassemblyWidget*>(dockWidget);
                disasm->setFocus();
            } else {
                break;
            }
        }
    }
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

void MainWindow::paintEvent(QPaintEvent *event)
{
    QMainWindow::paintEvent(event);
    /*
     * Dirty hack
     * Just to adjust the width of Functions Widget to fixed size
     * After everything is drawn, safely make it Preferred size policy
     * So that user can change the widget size with the mouse
     */
    if (functionsDock) {
        functionsDock->changeSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    }
}

void MainWindow::readSettingsOrDefault()
{
    QSettings settings;
    QByteArray geo = settings.value("geometry", QByteArray()).toByteArray();
    QByteArray state = settings.value("state", QByteArray()).toByteArray();
    /*
     * Check if saved settings exist
     * If not, then read the default layout
     */
    if (!geo.length() && !state.length()) {
        resetToDefaultLayout();
        return;
    }
    hideAllDocks();
    restoreGeometry(geo);
    restoreState(state);

    // make sure all DockWidgets are part of the MainWindow
    // also show them, so newly installed plugin widgets are shown right away
    for (auto dockWidget : dockWidgets) {
        if (dockWidgetArea(dockWidget) == Qt::DockWidgetArea::NoDockWidgetArea) {
            addDockWidget(Qt::DockWidgetArea::TopDockWidgetArea, dockWidget);
            dockWidget->show();
        }
    }

    responsive = settings.value("responsive").toBool();
    panelLock = settings.value("panelLock").toBool();
    setPanelLock();
    tabsOnTop = settings.value("tabsOnTop").toBool();
    setTabLocation();
    bool dockGroupedDragging = settings.value("docksGroupedDragging", false).toBool();
    ui->actionGrouped_dock_dragging->setChecked(dockGroupedDragging);
    on_actionGrouped_dock_dragging_triggered(dockGroupedDragging);

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
    settings.setValue("docksGroupedDragging", ui->actionGrouped_dock_dragging->isChecked());
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
    addDockWidget(Qt::TopDockWidgetArea, overviewDock);

    // Function | Dashboard
    splitDockWidget(overviewDock, dashboardDock, Qt::Horizontal);
    splitDockWidget(functionsDock, overviewDock, Qt::Vertical);

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
    tabifyDockWidget(dashboardDock, sdbDock);

    // Add Stack, Registers and Backtrace vertically stacked
    addDockWidget(Qt::TopDockWidgetArea, stackDock);
    splitDockWidget(stackDock, registersDock, Qt::Vertical);
    tabifyDockWidget(stackDock, backtraceDock);

    // MemoryMap/Breakpoint/RegRefs widget goes in the center tabs
    tabifyDockWidget(dashboardDock, memoryMapDock);
    tabifyDockWidget(dashboardDock, breakpointDock);
    tabifyDockWidget(dashboardDock, registerRefsDock);

    updateDockActionsChecked();
}


void MainWindow::hideAllDocks()
{
    for (auto w : dockWidgets) {
        removeDockWidget(w);
    }
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
}

void MainWindow::resetToDebugLayout()
{
    CutterCore::MemoryWidgetType memType = core->getMemoryWidgetPriority();
    hideAllDocks();
    restoreDocks();
    showDebugDocks();
    core->raisePrioritizedMemoryWidget(memType);

    auto restoreStackDock = qhelpers::forceWidth(stackDock->widget(), 400);
    qApp->processEvents();
    restoreStackDock.restoreWidth(stackDock->widget());
}

void MainWindow::restoreDebugLayout()
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
    RenameDialog r(this);
    // Get function based on click position
    //r->setFunctionName(fcn_name);
    r.exec();
}

void MainWindow::on_actionDefault_triggered()
{
    if (core->currentlyDebugging) {
        resetToDebugLayout();
    } else {
        resetToDefaultLayout();
    }
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

    const QString &fileName = QDir::toNativeSeparators(dialog.getOpenFileName(this,
                                                                              tr("Select radare2 script")));
    if (fileName.isEmpty()) // Cancel was pressed
        return;

    RunScriptTask *runScriptTask = new RunScriptTask();
    runScriptTask->setFileName(fileName);

    AsyncTask::Ptr runScriptTaskPtr(runScriptTask);

    AsyncTaskDialog *taskDialog = new AsyncTaskDialog(runScriptTaskPtr, this);
    taskDialog->setInterruptOnClose(true);
    taskDialog->setAttribute(Qt::WA_DeleteOnClose);
    taskDialog->show();

    Core()->getAsyncTaskManager()->start(runScriptTaskPtr);
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
        on_actionDefault_triggered();
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
    CommentsDialog c(this);
    c.exec();
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
    a->setAttribute(Qt::WA_DeleteOnClose);
    a->open();
}

void MainWindow::on_actionIssue_triggered()
{
    openIssue();
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

    const QString &pdbFile = QDir::toNativeSeparators(dialog.selectedFiles().first());

    if (!pdbFile.isEmpty()) {
        core->loadPDB(pdbFile);
        core->message(tr("%1 loaded.").arg(pdbFile));
        this->refreshAll();
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

void MainWindow::on_actionGrouped_dock_dragging_triggered(bool checked)
{
    auto options = dockOptions();
    options.setFlag(QMainWindow::DockOption::GroupedDragging, checked);
    setDockOptions(options);
}


void MainWindow::projectSaved(bool successfully, const QString &name)
{
    if (successfully)
        core->message(tr("Project saved: %1").arg(name));
    else
        core->message(tr("Failed to save project: %1").arg(name));
}

void MainWindow::changeDebugView()
{
    saveSettings();
    restoreDebugLayout();
    enableDebugWidgetsMenu(true);
}

void MainWindow::changeDefinedView()
{
    saveDebugSettings();
    CutterCore::MemoryWidgetType memType = core->getMemoryWidgetPriority();
    hideAllDocks();
    restoreDocks();
    readSettingsOrDefault();
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

/**
 * @brief Show a warning message box.
 *
 * This API can either be used in Cutter internals, or by Python plugins.
 */
void MainWindow::messageBoxWarning(QString title, QString message)
{
    QMessageBox mb(this);
    mb.setIcon(QMessageBox::Warning);
    mb.setStandardButtons(QMessageBox::Ok);
    mb.setWindowTitle(title);
    mb.setText(message);
    mb.exec();
}

/**
 * \brief When theme changed, change icons which have a special version for the theme.
 */
void MainWindow::chooseThemeIcons()
{
    // List of QActions which have alternative icons in different themes
    const QList<QPair<void*, QString>> kSupportedIconsNames {
        { ui->actionForward, QStringLiteral("arrow_right.svg") },
        { ui->actionBackward, QStringLiteral("arrow_left.svg") },      
    };


    // Set the correct icon for the QAction
    qhelpers::setThemeIcons(kSupportedIconsNames, [](void *obj, const QIcon &icon) {
        static_cast<QAction*>(obj)->setIcon(icon);
    });
}
