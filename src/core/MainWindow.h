#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// #include "core/Cutter.h" // only needed for ut64
#include "dialogs/NewFileDialog.h"
// #include "dialogs/WelcomeDialog.h"
#include "MemoryDockWidget.h"
#include "common/Configuration.h"
#include "common/CutterLayout.h"
#include "common/IOModesController.h"
#include "common/InitialOptions.h"

#include <QList>
#include <QMainWindow>

#include <memory>

class CutterCore;
class Omnibar;
class ProgressIndicator;
class PreviewWidget;
class Highlighter;
class AsciiHighlighter;
class VisualNavbar;
class FunctionsWidget;
class ImportsWidget;
class ExportsWidget;
class SymbolsWidget;
class GlobalsWidget;
class RelocsWidget;
class CommentsWidget;
class StringsWidget;
class FlagsWidget;
class Dashboard;
class QLineEdit;
class SdbWidget;
class QAction;
class SectionsWidget;
class SegmentsWidget;
class ConsoleWidget;
class EntrypointWidget;
class DisassemblerGraphView;
class ClassesWidget;
class ResourcesWidget;
class VTablesWidget;
class TypesWidget;
class HeadersWidget;
class FlirtWidget;
class SearchWidget;
class QDockWidget;
class DisassemblyWidget;
class GraphWidget;
class HexdumpWidget;
class DecompilerWidget;
class OverviewWidget;
class RizinGraphWidget;
class CallGraphWidget;
class HeapWidget;

namespace Ui {
class MainWindow;
}

/**
 * @brief Cutter main window
 */
class CUTTER_EXPORT MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    bool responsive;

    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void openNewFile(InitialOptions &options, bool skipOptionsDialog = false);
    void displayNewFileDialog();
    /**
     * @brief displays the WelocmeDialog
     * Upon first execution of Cutter, the WelcomeDialog would be showed to the user.
     * The Welcome dialog would be showed after a reset of Cutter's preferences by the user.
     */
    void displayWelcomeDialog();
    void closeNewFileDialog();
    bool openProject(const QString &project_name);

    RzProjectErr saveProject(bool *canceled);
    RzProjectErr saveProjectAs(bool *canceled);
    void showProjectSaveError(RzProjectErr err);

    void closeEvent(QCloseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void readSettings();
    void saveSettings();
    void setFilename(const QString &fn);

    void addWidget(CutterDockWidget *widget);
    void addMemoryDockWidget(MemoryDockWidget *widget);
    void removeWidget(CutterDockWidget *widget);
    void addExtraWidget(CutterDockWidget *extraDock);
    MemoryDockWidget *addNewMemoryWidget(MemoryWidgetType type, RVA address,
                                         bool synchronized = true);

    CUTTER_DEPRECATED("Action will be ignored. Use addPluginDockWidget(CutterDockWidget*) instead.")
    void addPluginDockWidget(CutterDockWidget *dockWidget, QAction *)
    {
        addPluginDockWidget(dockWidget);
    }
    void addPluginDockWidget(CutterDockWidget *dockWidget);
    enum class MenuType : ut8 { File, Edit, View, Windows, Debug, Help, Plugins };
    /**
     * @brief Getter for MainWindow's different menus
     * @param type The type which represents the desired menu
     * @return The requested menu or nullptr if "type" is invalid
     */
    QMenu *getMenuByType(MenuType type);
    void addMenuFileAction(QAction *action);

    QString getFilename() const { return filename; }
    /**
     * @brief Show a warning message box.
     * This API can either be used in Cutter internals, or by Python plugins.
     */
    void messageBoxWarning(const QString &title, const QString &message);

    QString getUniqueObjectName(const QString &widgetType) const;
    void showMemoryWidget();
    void showMemoryWidget(MemoryWidgetType type);
    QMenu *createShowInMenu(QWidget *parent, RVA address,
                            AddressTypeHint addressType = AddressTypeHint::Unknown);
    void setCurrentMemoryWidget(MemoryDockWidget *memoryWidget);
    MemoryDockWidget *getLastMemoryWidget();
    MemoryDockWidget *getOrCreateMemoryWidget(MemoryWidgetType type, RVA address = RVA_INVALID,
                                              bool synchronized = true);

    /* Context menu plugins */
    enum class ContextMenuType : ut8 { Disassembly, Addressable };
    /**
     * @brief Fetches the pointer to a context menu extension of type
     * @param type - the type of the context menu
     * @return plugins submenu of the selected context menu
     */
    QMenu *getContextMenuExtensions(ContextMenuType type);

public slots:
    void finalizeOpen();
    void showAddress(RVA addr);

    void refreshAll();
    void seekToFunctionLastInstruction();
    void seekToFunctionStart();
    void setTabLocation();

    void onActionTabsTriggered();

    /**
     * @brief A signal that creates an AsyncTask to re-analyze the current file
     */
    void onActionAnalyzeTriggered() const;

    void lockDocks(bool lock);

    void onActionRunScriptTriggered();

    void toggleResponsive(bool maybe);

    void openNewFileFailed();

    void toggleOverview(bool visibility, GraphWidget *targetGraph);
private slots:
    void onActionBaseFindTriggered();
    void onActionAboutTriggered();
    void onActionIssueTriggered();
    void documentationClicked();
    void addExtraGraph();
    void addExtraHexdump();
    void addExtraDisassembly();
    void addExtraDecompiler();

    void onActionRefreshPanelsTriggered();

    void onActionDisasAddCommentTriggered();

    void onActionDefaultTriggered();

    /**
     * @brief MainWindow::on_actionNew_triggered
     * Open a new Cutter session.
     */
    void onActionNewTriggered();

    void onActionSaveTriggered();
    void onActionSaveAsTriggered();

    void onActionBackwardTriggered();
    void onActionForwardTriggered();

    /**
     * @brief MainWindow::on_actionOpen_triggered
     * Open a file as in "load (add) a file in current session".
     */
    void onActionMapTriggered();

    void onActionTabsOnTopTriggered();

    void onActionResetSettingsTriggered();

    void onActionQuitTriggered();

    void onActionRefreshContentsTriggered();

    void onActionPreferencesTriggered();

    void onActionImportPdbTriggered();

    void onActionExportAsCodeTriggered();

    void onActionApplySigFromFileTriggered();

    void onActionCreateNewSigTriggered();

    void onActionGroupedDockDraggingTriggered(bool checked);

    void updateTasksIndicator();

    void mousePressEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event) override;
    bool event(QEvent *event) override;
    void toggleDebugView();
    /**
     * @brief When theme changed, change icons which have a special version for the theme.
     */
    void chooseThemeIcons();

    void onZoomIn();
    void onZoomOut();
    void onZoomReset();

    void setAvailableIOModeOptions();

private:
    CutterCore *core;

    bool tabsOnTop;
    ut64 hexdumpTopOffset;
    ut64 hexdumpBottomOffset;
    QString filename;
    std::unique_ptr<Ui::MainWindow> ui;
    Highlighter *highlighter;
    VisualNavbar *visualNavbar;
    Omnibar *omnibar;
    ProgressIndicator *tasksProgressIndicator;
    QByteArray emptyState;
    IOModesController ioModesController;

    Configuration *configuration;

    QList<CutterDockWidget *> dockWidgets;
    QList<CutterDockWidget *> pluginDocks;
    OverviewWidget *overviewDock = nullptr;
    QAction *actionOverview = nullptr;
    EntrypointWidget *entrypointDock = nullptr;
    FunctionsWidget *functionsDock = nullptr;
    ImportsWidget *importsDock = nullptr;
    ExportsWidget *exportsDock = nullptr;
    HeadersWidget *headersDock = nullptr;
    TypesWidget *typesDock = nullptr;
    SearchWidget *searchDock = nullptr;
    SymbolsWidget *symbolsDock = nullptr;
    GlobalsWidget *globalsDock = nullptr;
    RelocsWidget *relocsDock = nullptr;
    CommentsWidget *commentsDock = nullptr;
    StringsWidget *stringsDock = nullptr;
    FlagsWidget *flagsDock = nullptr;
    Dashboard *dashboardDock = nullptr;
    SdbWidget *sdbDock = nullptr;
    SectionsWidget *sectionsDock = nullptr;
    SegmentsWidget *segmentsDock = nullptr;
    FlirtWidget *flirtDock = nullptr;
    ConsoleWidget *consoleDock = nullptr;
    ClassesWidget *classesDock = nullptr;
    ResourcesWidget *resourcesDock = nullptr;
    VTablesWidget *vTablesDock = nullptr;
    CutterDockWidget *stackDock = nullptr;
    CutterDockWidget *threadsDock = nullptr;
    CutterDockWidget *processesDock = nullptr;
    CutterDockWidget *registersDock = nullptr;
    CutterDockWidget *backtraceDock = nullptr;
    CutterDockWidget *memoryMapDock = nullptr;
    NewFileDialog *newFileDialog = nullptr;
    CutterDockWidget *breakpointDock = nullptr;
    CutterDockWidget *registerRefsDock = nullptr;
    RizinGraphWidget *rzGraphDock = nullptr;
    CallGraphWidget *callGraphDock = nullptr;
    CallGraphWidget *globalCallGraphDock = nullptr;
    CutterDockWidget *heapDock = nullptr;

    QMenu *disassemblyContextMenuExtensions = nullptr;
    QMenu *addressableContextMenuExtensions = nullptr;

    QMap<QString, Cutter::CutterLayout> layouts;

    void initUI();
    void initToolBar();
    void initDocks();
    void initBackForwardMenu();
    void displayInitialOptionsDialog(const InitialOptions &options = InitialOptions(),
                                     bool skipOptionsDialog = false);

    Cutter::CutterLayout getViewLayout();
    Cutter::CutterLayout getViewLayout(const QString &name);

    void setViewLayout(const Cutter::CutterLayout &layout);
    void loadLayouts(QSettings &settings);
    void saveLayouts(QSettings &settings);

    void updateMemberPointers();
    void restoreDocks();
    void showZenDocks();
    void showDebugDocks();
    /**
     * @brief Try to guess which is the "main" section of layout and dock there.
     * @param widget that needs to be docked
     */
    void dockOnMainArea(QDockWidget *widget);
    void enableDebugWidgetsMenu(bool enable);
    /**
     * @brief Fill menu with seek history entries.
     * @param menu Menu to update
     * @param redo set to false for undo history, true for redo.
     */
    void updateHistoryMenu(QMenu *menu, bool redo = false);
    void updateLayoutsMenu();
    void saveNamedLayout();
    void manageLayouts();

    void setOverviewData();
    bool isOverviewActive();
    /**
     * @brief Check if a widget is one of debug specific dock widgets.
     * @param dock DockWidget to check
     * @return true for debug specific widgets, false for all other including common dock widgets.
     */
    bool isDebugWidget(QDockWidget *dock) const;
    bool isExtraMemoryWidget(QDockWidget *dock) const;

    MemoryWidgetType getMemoryWidgetTypeToRestore();

    /**
     * @brief Map from a widget type (e.g. DisassemblyWidget::getWidgetType()) to the respective
     * contructor of the widget
     */
    QMap<QString, std::function<CutterDockWidget *(MainWindow *)>> widgetTypeToConstructorMap;

    MemoryDockWidget *lastSyncMemoryWidget = nullptr;
    MemoryDockWidget *lastMemoryWidget = nullptr;
    int functionDockWidthToRestore = 0;
};

#endif // MAINWINDOW_H
