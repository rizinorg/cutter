#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "core/Cutter.h" // only needed for ut64
#include "dialogs/NewFileDialog.h"
#include "dialogs/WelcomeDialog.h"
#include "common/Configuration.h"
#include "common/InitialOptions.h"
#include "MemoryDockWidget.h"

#include <memory>

#include <QMainWindow>
#include <QList>

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
class ZignaturesWidget;
class SearchWidget;
class QDockWidget;
class DisassemblyWidget;
class GraphWidget;
class HexdumpWidget;
class DecompilerWidget;
class OverviewWidget;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    bool responsive;

    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void openNewFile(InitialOptions &options, bool skipOptionsDialog = false);
    void displayNewFileDialog();
    void displayWelcomeDialog();
    void closeNewFileDialog();
    void openProject(const QString &project_name);

    void initUI();

    /**
     * @param quit whether to show destructive button in dialog
     * @return if quit is true, false if the application should not close
     */
    bool saveProject(bool quit = false);

    /**
     * @param quit whether to show destructive button in dialog
     * @return false if the application should not close
     */
    bool saveProjectAs(bool quit = false);

    void closeEvent(QCloseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void readSettingsOrDefault();
    void saveSettings();
    void readDebugSettings();
    void saveDebugSettings();
    void setFilename(const QString &fn);
    void refreshOmniBar(const QStringList &flags);

    void addWidget(QDockWidget *widget);
    void addMemoryDockWidget(MemoryDockWidget *widget);
    void removeWidget(QDockWidget *widget);
    void addExtraWidget(CutterDockWidget *extraDock);
    MemoryDockWidget *addNewMemoryWidget(MemoryWidgetType type, RVA address, bool synchronized = true);


    void addPluginDockWidget(QDockWidget *dockWidget, QAction *action);
    enum class MenuType { File, Edit, View, Windows, Debug, Help, Plugins };
    QMenu *getMenuByType(MenuType type);
    void addMenuFileAction(QAction *action);

    void updateDockActionChecked(QAction * action);

    QString getFilename() const
    {
        return filename;
    }
    void messageBoxWarning(QString title, QString message);

    QString getUniqueObjectName(const QString &widgetType) const;
    void showMemoryWidget();
    void showMemoryWidget(MemoryWidgetType type);

    QMenu *createShowInMenu(QWidget *parent, RVA address);
    void setCurrentMemoryWidget(MemoryDockWidget* memoryWidget);
    MemoryDockWidget* getLastMemoryWidget();

public slots:
    void finalizeOpen();

    void refreshAll();
    void seekToFunctionLastInstruction();
    void seekToFunctionStart();
    void setPanelLock();
    void setTabLocation();

    void on_actionLock_triggered();

    void on_actionLockUnlock_triggered();

    void on_actionTabs_triggered();

    void lockUnlock_Docks(bool what);

    void on_actionRun_Script_triggered();

    void toggleResponsive(bool maybe);

    void openNewFileFailed();

    void toggleOverview(bool visibility, GraphWidget *targetGraph);
private slots:
    void on_actionAbout_triggered();
    void on_actionIssue_triggered();
    void addExtraGraph();
    void addExtraHexdump();
    void addExtraDisassembly();

    void on_actionRefresh_Panels_triggered();

    void on_actionDisasAdd_comment_triggered();

    void on_actionDefault_triggered();

    void on_actionFunctionsRename_triggered();

    void on_actionNew_triggered();

    void on_actionSave_triggered();
    void on_actionSaveAs_triggered();

    void on_actionBackward_triggered();
    void on_actionForward_triggered();
    void on_actionUndoSeek_triggered();
    void on_actionRedoSeek_triggered();

    void on_actionOpen_triggered();

    void on_actionTabs_on_Top_triggered();

    void on_actionReset_settings_triggered();

    void on_actionQuit_triggered();

    void on_actionRefresh_contents_triggered();

    void on_actionPreferences_triggered();

    void on_actionAnalyze_triggered();

    void on_actionImportPDB_triggered();

    void on_actionExport_as_code_triggered();

    void on_actionGrouped_dock_dragging_triggered(bool checked);

    void projectSaved(bool successfully, const QString &name);

    void updateTasksIndicator();

    void mousePressEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event) override;
    void changeDebugView();
    void changeDefinedView();
    void chooseThemeIcons();

private:
    CutterCore *core;

    bool panelLock;
    bool tabsOnTop;
    ut64 hexdumpTopOffset;
    ut64 hexdumpBottomOffset;
    QString filename;
    std::unique_ptr<Ui::MainWindow> ui;
    Highlighter *highlighter;
    AsciiHighlighter *hex_highlighter;
    VisualNavbar *visualNavbar;
    Omnibar *omnibar;
    ProgressIndicator *tasksProgressIndicator;
    QByteArray emptyState;

    Configuration *configuration;

    QList<QDockWidget *> dockWidgets;
    QMultiMap<QAction *, QDockWidget *> dockWidgetsOfAction;
    DecompilerWidget   *decompilerDock = nullptr;
    OverviewWidget     *overviewDock = nullptr;
    EntrypointWidget   *entrypointDock = nullptr;
    FunctionsWidget    *functionsDock = nullptr;
    ImportsWidget      *importsDock = nullptr;
    ExportsWidget      *exportsDock = nullptr;
    HeadersWidget      *headersDock = nullptr;
    TypesWidget        *typesDock = nullptr;
    SearchWidget       *searchDock = nullptr;
    SymbolsWidget      *symbolsDock = nullptr;
    RelocsWidget       *relocsDock = nullptr;
    CommentsWidget     *commentsDock = nullptr;
    StringsWidget      *stringsDock = nullptr;
    FlagsWidget        *flagsDock = nullptr;
    Dashboard          *dashboardDock = nullptr;
    QLineEdit          *gotoEntry = nullptr;
    SdbWidget          *sdbDock = nullptr;
    SectionsWidget     *sectionsDock = nullptr;
    SegmentsWidget     *segmentsDock = nullptr;
    ZignaturesWidget   *zignaturesDock = nullptr;
    ConsoleWidget      *consoleDock = nullptr;
    ClassesWidget      *classesDock = nullptr;
    ResourcesWidget    *resourcesDock = nullptr;
    VTablesWidget      *vTablesDock = nullptr;
    DisassemblerGraphView *graphView = nullptr;
    QDockWidget        *asmDock = nullptr;
    QDockWidget        *calcDock = nullptr;
    QDockWidget        *stackDock = nullptr;
    QDockWidget        *registersDock = nullptr;
    QDockWidget        *backtraceDock = nullptr;
    QDockWidget        *memoryMapDock = nullptr;
    NewFileDialog      *newFileDialog = nullptr;
    QDockWidget        *breakpointDock = nullptr;
    QDockWidget        *registerRefsDock = nullptr;

    void initToolBar();
    void initDocks();
    void initLayout();
    void initCorners();
    void displayInitialOptionsDialog(const InitialOptions &options = InitialOptions(), bool skipOptionsDialog = false);

    void resetToDefaultLayout();
    void resetToDebugLayout();
    void restoreDebugLayout();

    void updateMemberPointers();
    void resetDockWidgetList();
    void restoreDocks();
    void hideAllDocks();
    void showZenDocks();
    void showDebugDocks();
    void enableDebugWidgetsMenu(bool enable);

    void toggleDockWidget(QDockWidget *dock_widget, bool show);

    void updateDockActionsChecked();
    void setOverviewData();
    bool isOverviewActive();

    MemoryWidgetType getMemoryWidgetTypeToRestore();

    /**
     * @brief Map from a widget type (e.g. DisassemblyWidget::getWidgetType()) to the respective contructor of the widget
     */
    QMap<QString, std::function<CutterDockWidget*(MainWindow*, QAction*)>> widgetTypeToConstructorMap;

    MemoryDockWidget* lastSyncMemoryWidget = nullptr;
    MemoryDockWidget* lastMemoryWidget = nullptr;
};

#endif // MAINWINDOW_H
