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
#include "widgets/ThreadsWidget.h"
#include "widgets/ProcessesWidget.h"
#include "widgets/RegistersWidget.h"
#include "widgets/BacktraceWidget.h"
#include "widgets/HexdumpWidget.h"
#include "widgets/DecompilerWidget.h"
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
#include <QJsonArray>

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

template<class T>
T* getNewInstance(MainWindow *m, QAction *a) { return new T(m, a); }

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    core(Core()),
    ui(new Ui::MainWindow)
{
    panelLock = false;
    tabsOnTop = false;
    configuration = Config();

    initUI();
}

MainWindow::~MainWindow()
{
}

void MainWindow::initUI()
{
    ui->setupUi(this);

    connect(ui->actionExtraGraph, &QAction::triggered, this, &MainWindow::addExtraGraph);
    connect(ui->actionExtraDisassembly, &QAction::triggered, this, &MainWindow::addExtraDisassembly);
    connect(ui->actionExtraHexdump, &QAction::triggered, this, &MainWindow::addExtraHexdump);

    widgetTypeToConstructorMap.insert(GraphWidget::getWidgetType(), getNewInstance<GraphWidget>);
    widgetTypeToConstructorMap.insert(DisassemblyWidget::getWidgetType(), getNewInstance<DisassemblyWidget>);
    widgetTypeToConstructorMap.insert(HexdumpWidget::getWidgetType(), getNewInstance<HexdumpWidget>);

    initToolBar();
    initDocks();

    emptyState = saveState();
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
    QShortcut *seek_to_func_end_shortcut = new QShortcut(QKeySequence(Qt::Key_Dollar), this);
    connect(seek_to_func_end_shortcut, SIGNAL(activated()), SLOT(seekToFunctionLastInstruction()));
    QShortcut *seek_to_func_start_shortcut = new QShortcut(QKeySequence(Qt::Key_AsciiCircum), this);
    connect(seek_to_func_start_shortcut, SIGNAL(activated()), SLOT(seekToFunctionStart()));

    QShortcut *refresh_shortcut = new QShortcut(QKeySequence(QKeySequence::Refresh), this);
    connect(refresh_shortcut, SIGNAL(activated()), this, SLOT(refreshAll()));

    connect(ui->actionZoomIn, SIGNAL(triggered()), this, SLOT(onZoomIn()));
    connect(ui->actionZoomOut, SIGNAL(triggered()), this, SLOT(onZoomOut()));
    connect(ui->actionZoomReset, SIGNAL(triggered()), this, SLOT(onZoomReset()));

    connect(core, SIGNAL(projectSaved(bool, const QString &)), this, SLOT(projectSaved(bool,
                                                                                       const QString &)));

    connect(core, &CutterCore::toggleDebugView, this, &MainWindow::toggleDebugView);

    connect(core, SIGNAL(newMessage(const QString &)),
            this->consoleDock, SLOT(addOutput(const QString &)));
    connect(core, SIGNAL(newDebugMessage(const QString &)),
            this->consoleDock, SLOT(addDebugOutput(const QString &)));

    connect(core, &CutterCore::showMemoryWidgetRequested,
            this, static_cast<void(MainWindow::*)()>(&MainWindow::showMemoryWidget));

    updateTasksIndicator();
    connect(core->getAsyncTaskManager(), &AsyncTaskManager::tasksChanged, this,
            &MainWindow::updateTasksIndicator);

    //Undo and redo seek
    ui->actionBackward->setShortcut(QKeySequence::Back);
    ui->actionForward->setShortcut(QKeySequence::Forward);

    initBackForwardMenu();

    /* Setup plugins interfaces */
    for (auto plugin : Plugins()->getPlugins()) {
        plugin->setupInterface(this);
    }

#if QT_VERSION < QT_VERSION_CHECK(5, 7, 0)
    ui->actionGrouped_dock_dragging->setVisible(false);
#endif

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
    auto debugViewAction = ui->menuDebug->addAction(tr("View"));
    debugViewAction->setMenu(ui->menuAddDebugWidgets);
    ui->menuDebug->addSeparator();
    ui->menuDebug->addAction(debugActions->actionStart);
    ui->menuDebug->addAction(debugActions->actionStartEmul);
    ui->menuDebug->addAction(debugActions->actionAttach);
    ui->menuDebug->addAction(debugActions->actionStartRemote);
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
    decompilerDock = new DecompilerWidget(this, ui->actionDecompiler);
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
    threadsDock = new ThreadsWidget(this, ui->actionThreads);
    processesDock = new ProcessesWidget(this, ui->actionProcesses);
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

    QSettings s;
    QStringList docks = s.value("docks", QStringList {
        DisassemblyWidget::getWidgetType(),
        GraphWidget::getWidgetType(),
        HexdumpWidget::getWidgetType()
    }).toStringList();

    // Restore all extra widgets
    QString className;
    for (const auto &it : docks) {
        if (std::none_of(dockWidgets.constBegin(), dockWidgets.constEnd(),
        [&it](QDockWidget * w) { return w->objectName() == it; })) {
            className = it.split(';').at(0);
            if (widgetTypeToConstructorMap.contains(className)) {
                auto widget = widgetTypeToConstructorMap[className](this, nullptr);
                widget->setObjectName(it);
                addExtraWidget(widget);
            }
        }
    }
}

void MainWindow::initLayout()
{
    // Set up dock widgets default layout
    enableDebugWidgetsMenu(false);
    // Restore saved settings
    readSettingsOrDefault();

    initCorners();
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

void MainWindow::addExtraGraph()
{
    auto *extraDock = new GraphWidget(this, nullptr);
    addExtraWidget(extraDock);
}

void MainWindow::addExtraHexdump()
{
    auto *extraDock = new HexdumpWidget(this, nullptr);
    addExtraWidget(extraDock);
}

void MainWindow::addExtraDisassembly()
{
    auto *extraDock = new DisassemblyWidget(this, nullptr);
    addExtraWidget(extraDock);
}

void MainWindow::addExtraWidget(CutterDockWidget *extraDock)
{
    extraDock->setTransient(true);
    addDockWidget(Qt::TopDockWidgetArea, extraDock, Qt::Orientation::Horizontal);
    auto restoreExtraDock = qhelpers::forceWidth(extraDock->widget(), 600);
    qApp->processEvents();
    restoreExtraDock.restoreWidth(extraDock->widget());
}

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
    dockWidget->addAction(action);
    addWidget(dockWidget);
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


    QSettings s;
    QStringList unsync = s.value("unsync").toStringList();
    for (auto it : dockWidgets) {
        auto w = qobject_cast<MemoryDockWidget*>(it);
        if (w) {
            w->getSeekable()->setSynchronization(!unsync.contains(it->objectName()));
        }
    }

    // Set focus to disasm or graph widget

    // Use for loop to cover cases when main disasm/graph
    // (MainWindow::disassemblyDock and MainWindow::graphDock)
    // widgets are invisible but extra ones are visible

    // Graph with function in it has focus priority over DisasmWidget
    // if there are both graph and disasm.
    // Otherwise Disasm has focus priority over Graph

    // If there are no graph/disasm widgets focus on MainWindow

    setFocus();
    bool graphContainsFunc = false;
    for (auto dockWidget : dockWidgets) {
        const QString className = dockWidget->metaObject()->className();
        auto graphWidget = qobject_cast<GraphWidget*>(dockWidget);
        if (graphWidget && !dockWidget->visibleRegion().isNull()) {
            graphContainsFunc = !graphWidget->getGraphView()->getBlocks().empty();
            if (graphContainsFunc) {
                dockWidget->widget()->setFocus();
                break;
            }
        }
        auto disasmWidget = qobject_cast<DisassemblyWidget*>(dockWidget);
        if (disasmWidget && !dockWidget->visibleRegion().isNull()) {
            if (!graphContainsFunc) {
                disasmWidget->setFocus();
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
    if (!geo.length() || !state.length()) {
        resetToDefaultLayout();
        return;
    }
    hideAllDocks();
    restoreGeometry(geo);
    restoreState(state);

    // make sure all DockWidgets are part of the MainWindow
    // also show them, so newly installed plugin widgets are shown right away
    for (auto dockWidget : dockWidgets) {
        if (dockWidgetArea(dockWidget) == Qt::DockWidgetArea::NoDockWidgetArea &&
            !isDebugWidget(dockWidget)) {
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

    QStringList docks;
    QStringList unsync;
    for (const auto &it : dockWidgets) {
        docks.append(it->objectName());
        auto memoryDockWidget = qobject_cast<MemoryDockWidget*>(it);
        if (memoryDockWidget && !memoryDockWidget->getSeekable()->isSynchronized()) {
            unsync.append(it->objectName());
        }
    }
    settings.setValue("docks", docks);
    settings.setValue("unsync", unsync);
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
    splitDockWidget(functionsDock, dashboardDock, Qt::Horizontal);
    tabifyDockWidget(dashboardDock, decompilerDock);
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
    tabifyDockWidget(dashboardDock, memoryMapDock);
    tabifyDockWidget(dashboardDock, breakpointDock);
    tabifyDockWidget(dashboardDock, registerRefsDock);
    for (const auto &it : dockWidgets) {
        // Check whether or not current widgets is graph, hexdump or disasm
        if (qobject_cast<GraphWidget*>(it) ||
            qobject_cast<HexdumpWidget*>(it) ||
            qobject_cast<DisassemblyWidget*>(it)) {
            tabifyDockWidget(dashboardDock, it);
        }
    }

    splitDockWidget(functionsDock, overviewDock, Qt::Vertical);

    // In the lower half the console is the first widget
    addDockWidget(Qt::BottomDockWidgetArea, consoleDock);

    // Console | Sections
    splitDockWidget(consoleDock, sectionsDock, Qt::Horizontal);
    splitDockWidget(consoleDock, segmentsDock, Qt::Horizontal);

    tabifyDockWidget(sectionsDock, commentsDock);

    // Add Stack, Registers, Threads and Backtrace vertically stacked
    addDockWidget(Qt::TopDockWidgetArea, stackDock);
    splitDockWidget(stackDock, registersDock, Qt::Vertical);
    tabifyDockWidget(stackDock, backtraceDock);
    tabifyDockWidget(backtraceDock, threadsDock);
    tabifyDockWidget(threadsDock, processesDock);

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
    for (auto i = dockWidgetsOfAction.constBegin(); i != dockWidgetsOfAction.constEnd(); i++) {
        updateDockActionChecked(i.key());
    }
}

bool MainWindow::isDebugWidget(QDockWidget *dock) const
{
    return dock == stackDock ||
           dock == registersDock ||
           dock == backtraceDock ||
           dock == threadsDock ||
           dock == memoryMapDock ||
           dock == breakpointDock ||
           dock == processesDock ||
           dock == registerRefsDock;
}

MemoryWidgetType MainWindow::getMemoryWidgetTypeToRestore()
{
    if (lastSyncMemoryWidget) {
        return lastSyncMemoryWidget->getType();
    }
    return MemoryWidgetType::Disassembly;
}

QString MainWindow::getUniqueObjectName(const QString &widgetType) const
{
    QStringList docks;
    docks.reserve(dockWidgets.size());
    QString name;
    for (const auto &it : dockWidgets) {
        name = it->objectName();
        if (name.split(';').at(0) == widgetType) {
            docks.push_back(name);
        }
    }

    if (docks.isEmpty()) {
        return widgetType;
    }

    int id = 0;
    while (docks.contains(widgetType + ";" + QString::number(id))) {
        id++;
    }

    return widgetType + ";"  + QString::number(id);
}

void MainWindow::showMemoryWidget()
{
    if (lastSyncMemoryWidget) {
        if (lastSyncMemoryWidget->tryRaiseMemoryWidget()) {
            return;
        }
    }
    showMemoryWidget(MemoryWidgetType::Disassembly);
}

void MainWindow::showMemoryWidget(MemoryWidgetType type)
{
    for (auto &dock : dockWidgets) {
        if (auto memoryWidget = qobject_cast<MemoryDockWidget *>(dock)) {
            if (memoryWidget->getType() == type && memoryWidget->getSeekable()->isSynchronized()) {
                memoryWidget->tryRaiseMemoryWidget();
                return;
            }
        }
    }
    auto memoryDockWidget = addNewMemoryWidget(type, Core()->getOffset());
    memoryDockWidget->raiseMemoryWidget();
}

QMenu *MainWindow::createShowInMenu(QWidget *parent, RVA address)
{
    QMenu *menu = new QMenu(parent);
    for (auto &dock : dockWidgets) {
        if (auto memoryWidget = qobject_cast<MemoryDockWidget *>(dock)) {
            QAction *action = new QAction(memoryWidget->windowTitle(), menu);
            connect(action, &QAction::triggered, this, [this, memoryWidget, address](){
                memoryWidget->getSeekable()->seek(address);
                memoryWidget->raiseMemoryWidget();
            });
            menu->addAction(action);
        }
    }
    menu->addSeparator();
    auto createAddNewWidgetAction = [this, menu, address](QString label, MemoryWidgetType type) {
        QAction *action = new QAction(label, menu);
        connect(action, &QAction::triggered, this, [this, address, type](){
            addNewMemoryWidget(type, address, false);
        });
        menu->addAction(action);
    };
    createAddNewWidgetAction(tr("New disassembly"), MemoryWidgetType::Disassembly);
    createAddNewWidgetAction(tr("New graph"), MemoryWidgetType::Graph);
    createAddNewWidgetAction(tr("New hexdump"), MemoryWidgetType::Hexdump);

    return menu;
}

void MainWindow::setCurrentMemoryWidget(MemoryDockWidget *memoryWidget)
{
    if (memoryWidget->getSeekable()->isSynchronized()) {
        lastSyncMemoryWidget = memoryWidget;
    }
    lastMemoryWidget = memoryWidget;
}

MemoryDockWidget *MainWindow::getLastMemoryWidget()
{
    return lastMemoryWidget;
}

MemoryDockWidget *MainWindow::addNewMemoryWidget(MemoryWidgetType type, RVA address,
                                                 bool synchronized)
{
    MemoryDockWidget *memoryWidget = nullptr;
    switch (type) {
    case MemoryWidgetType::Graph:
        memoryWidget = new GraphWidget(this);
        break;
    case MemoryWidgetType::Hexdump:
        memoryWidget = new HexdumpWidget(this);
        break;
    case MemoryWidgetType::Disassembly:
        memoryWidget = new DisassemblyWidget(this);
        break;
    case MemoryWidgetType::Decompiler:
        memoryWidget = new DecompilerWidget(this);
        break;
    }
    auto seekable = memoryWidget->getSeekable();
    seekable->setSynchronization(synchronized);
    seekable->seek(address);
    addExtraWidget(memoryWidget);
    return memoryWidget;
}

void MainWindow::initCorners()
{
    // TODO: Allow the user to select this option visually in the GUI settings
    // Adjust the DockWidget areas

    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);

    setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
}

void MainWindow::initBackForwardMenu()
{
    auto prepareButtonMenu = [this](QAction *action) -> QMenu* {
        QToolButton *button = qobject_cast<QToolButton *>(ui->mainToolBar->widgetForAction(action));
        if (!button) {
            return nullptr;
        }
        QMenu *menu = new QMenu(button);
        button->setMenu(menu);
        button->setPopupMode(QToolButton::DelayedPopup);
        button->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(button, &QWidget::customContextMenuRequested, button,
                [menu, button] (const QPoint &pos) {
            menu->exec(button->mapToGlobal(pos));
        });

        QFontMetrics metrics(fontMetrics());
        // Roughly 10-16 lines depending on padding size, no need to calculate more precisely
        menu->setMaximumHeight(metrics.lineSpacing() * 20);

        menu->setToolTipsVisible(true);
        return menu;
    };

    if (auto menu = prepareButtonMenu(ui->actionBackward)) {
        menu->setObjectName("historyMenu");
        connect(menu, &QMenu::aboutToShow, menu, [this, menu]() {
            updateHistoryMenu(menu, false);
        });
    }
    if (auto menu = prepareButtonMenu(ui->actionForward)) {
        menu->setObjectName("forwardHistoryMenu");
        connect(menu, &QMenu::aboutToShow, menu, [this, menu]() {
            updateHistoryMenu(menu, true);
        });
    }
}

void MainWindow::updateHistoryMenu(QMenu *menu, bool redo)
{
    // Not too long so that whole screen doesn't get covered,
    // not too short so that reasonable length c++ names can be seen most of the time
    const int MAX_NAME_LENGTH = 64;

    auto hist = Core()->cmdj("sj");
    bool history = true;
    QList<QAction *> actions;
    for (auto item : Core()->cmdj("sj").array()) {
        QJsonObject obj = item.toObject();
        QString name = obj["name"].toString();
        RVA offset = obj["offset"].toVariant().toULongLong();
        bool current = obj["current"].toBool(false);
        if (current) {
            history = false;
        }
        if (history != redo || current) { // Include current in both directions
            QString addressString = RAddressString(offset);

            QString toolTip = QString("%1 %2").arg(addressString, name); // show non truncated name in tooltip

            name.truncate(MAX_NAME_LENGTH); // TODO:#1904 use common name shortening function
            QString label = QString("%1 (%2)").arg(name, addressString);
            if (current) {
                label = QString("current position (%1)").arg(addressString);
            }
            QAction *action = new QAction(label, menu);
            action->setToolTip(toolTip);
            actions.push_back(action);
            if (current) {
                QFont font;
                font.setBold(true);
                action->setFont(font);
            }
        }
    }
    if (!redo) {
        std::reverse(actions.begin(), actions.end());
    }
    menu->clear();
    menu->addActions(actions);
    int steps = 0;
    for (QAction *item : menu->actions()) {
        if (redo) {
            connect(item, &QAction::triggered, item, [steps]() {
                for (int i = 0; i < steps; i++) {
                    Core()->seekNext();
                }
            });
        } else {
            connect(item, &QAction::triggered, item, [steps]() {
                for (int i = 0; i < steps; i++) {
                    Core()->seekPrev();
                }
            });
        }
        ++steps;
    }

}

void MainWindow::addWidget(QDockWidget* widget)
{
    dockWidgets.push_back(widget);
    for (auto action : widget->actions()) {
        dockWidgetsOfAction.insert(action, widget);
        connect(qobject_cast<CutterDockWidget*>(widget), &CutterDockWidget::closed,
                this, [this]() {
            QDockWidget *widget = qobject_cast<QDockWidget*>(sender());
            dockWidgets.removeOne(widget);
            for (auto action : widget->actions()) {
                dockWidgetsOfAction.remove(action, widget);
            }
            updateDockActionsChecked();
        });
    }    
}

void MainWindow::addMemoryDockWidget(MemoryDockWidget *widget)
{
    connect(widget, &QDockWidget::visibilityChanged, this, [this, widget](bool visibility) {
        if (visibility) {
            setCurrentMemoryWidget(widget);
        }
    });
}

void MainWindow::removeWidget(QDockWidget *widget)
{
    dockWidgets.removeAll(widget);
    if (lastSyncMemoryWidget == widget) {
        lastSyncMemoryWidget = nullptr;
    }
    if (lastMemoryWidget == widget) {
        lastMemoryWidget = nullptr;
    }
}

void MainWindow::updateDockActionChecked(QAction *action)
{
    auto actions = dockWidgetsOfAction.values(action);
    action->setChecked(!std::accumulate(actions.begin(), actions.end(), false,
                                       [](bool a, QDockWidget* w) -> bool {
        return a || w->isHidden();
    }));
}

void MainWindow::showZenDocks()
{
    const QList<QDockWidget *> zenDocks = { functionsDock,
                                            dashboardDock,
                                            stringsDock,
                                            searchDock,
                                            importsDock
                                          };
    int width = functionsDock->maximumWidth();
    functionsDock->setMaximumWidth(200);
    for (auto w : dockWidgets) {
        if (zenDocks.contains(w) ||
            qobject_cast<GraphWidget*>(w) ||
            qobject_cast<HexdumpWidget*>(w) ||
            qobject_cast<DisassemblyWidget*>(w)) {
            w->show();
        }
    }
    functionsDock->setMaximumWidth(width);
    updateDockActionsChecked();
}

void MainWindow::showDebugDocks()
{
    const QList<QDockWidget *> debugDocks = { functionsDock,
                                              stringsDock,
                                              searchDock,
                                              stackDock,
                                              registersDock,
                                              backtraceDock,
                                              threadsDock,
                                              memoryMapDock,
                                              breakpointDock
                                            };
    int width = functionsDock->maximumWidth();
    functionsDock->setMaximumWidth(200);
    for (auto w : dockWidgets) {
        if (debugDocks.contains(w) ||
            qobject_cast<GraphWidget*>(w) ||
            qobject_cast<HexdumpWidget*>(w) ||
            qobject_cast<DisassemblyWidget*>(w)) {
            w->show();
        }
    }
    functionsDock->setMaximumWidth(width);
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
    MemoryWidgetType memType = getMemoryWidgetTypeToRestore();
    hideAllDocks();
    restoreDocks();
    showDebugDocks();
    showMemoryWidget(memType);

    auto restoreStackDock = qhelpers::forceWidth(stackDock->widget(), 400);
    qApp->processEvents();
    restoreStackDock.restoreWidth(stackDock->widget());
}

void MainWindow::restoreDebugLayout()
{
    MemoryWidgetType memType = getMemoryWidgetTypeToRestore();
    bool isMaxim = isMaximized();
    hideAllDocks();
    restoreDocks();
    showDebugDocks();
    readDebugSettings();
    showMemoryWidget(memType);
    if (isMaxim) {
        showMaximized();
    } else {
        showNormal();
    }
}

void MainWindow::resetDockWidgetList()
{
    QStringList isLeft;
    QList<QWidget*> toClose;
    for (auto it : dockWidgets) {
        if (isLeft.contains(it->metaObject()->className())) {
            toClose.append(it);
        } else if (QRegularExpression("\\A(?:\\w+ \\d+)\\z").match(it->objectName()).hasMatch()) {
            isLeft.append(it->metaObject()->className());
        }
    }
    for (auto it : toClose) {
        it->close();
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
    QSettings s;
    restoreState(emptyState);

    initCorners();
    resetDockWidgetList();

    if (core->currentlyDebugging) {
        resetToDefaultLayout();
        saveSettings();

        resetToDebugLayout();
    } else {
        resetToDebugLayout();
        saveDebugSettings();

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
#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    auto options = dockOptions();
    options.setFlag(QMainWindow::DockOption::GroupedDragging, checked);
    setDockOptions(options);
#else
    Q_UNUSED(checked);
#endif
}

void MainWindow::seekToFunctionLastInstruction()
{
    Core()->seek(Core()->getLastFunctionInstruction(Core()->getOffset()));
}

void MainWindow::seekToFunctionStart()
{
    Core()->seek(Core()->getFunctionStart(Core()->getOffset()));
}

void MainWindow::projectSaved(bool successfully, const QString &name)
{
    if (successfully)
        core->message(tr("Project saved: %1").arg(name));
    else
        core->message(tr("Failed to save project: %1").arg(name));
}

void MainWindow::toggleDebugView()
{
    if (Core()->currentlyDebugging) {
        saveSettings();
        restoreDebugLayout();
        enableDebugWidgetsMenu(true);
    } else {
        saveDebugSettings();
        MemoryWidgetType memType = getMemoryWidgetTypeToRestore();
        hideAllDocks();
        restoreDocks();
        readSettingsOrDefault();
        enableDebugWidgetsMenu(false);
        showMemoryWidget(memType);
    }
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

bool MainWindow::event(QEvent *event)
{
    if (event->type() == QEvent::FontChange
        || event->type() == QEvent::StyleChange
        || event->type() == QEvent::PaletteChange) {
#if QT_VERSION < QT_VERSION_CHECK(5,10,0)
        QMetaObject::invokeMethod(Config(), "refreshFont", Qt::ConnectionType::QueuedConnection);
#else
        QMetaObject::invokeMethod(Config(), &Configuration::refreshFont, Qt::ConnectionType::QueuedConnection);
#endif
    }
    return QMainWindow::event(event);
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
 * @brief When theme changed, change icons which have a special version for the theme.
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

void MainWindow::onZoomIn()
{
  Config()->setZoomFactor(Config()->getZoomFactor() + 0.1);
}

void MainWindow::onZoomOut()
{
  Config()->setZoomFactor(Config()->getZoomFactor() - 0.1);
}

void MainWindow::onZoomReset()
{
  Config()->setZoomFactor(1.0);
}
