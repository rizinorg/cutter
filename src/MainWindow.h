#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>

#include "Cutter.h" // only needed for ut64
#include "widgets/DisassemblyWidget.h"
#include "widgets/SidebarWidget.h"
#include "widgets/StackWidget.h"
#include "widgets/RegistersWidget.h"
#include "widgets/BacktraceWidget.h"
#include "widgets/HexdumpWidget.h"
#include "widgets/PseudocodeWidget.h"
#include "dialogs/NewFileDialog.h"
#include "utils/Configuration.h"

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
class SdbDock;
class QAction;
class SectionsWidget;
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
#ifdef CUTTER_ENABLE_JUPYTER
class JupyterWidget;
#endif
class QDockWidget;

namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    bool responsive;

    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void openNewFile(const QString &fn, int analLevel = -1,
                     QList<QString> advancedOptions = QList<QString>());
    void displayNewFileDialog();
    void closeNewFileDialog();
    void displayAnalysisOptionsDialog(int analLevel, QList<QString> advancedOptions);
    void openProject(const QString &project_name);

    void initUI();

    /*!
     * @param quit whether to show destructive button in dialog
     * @return if quit is true, false if the application should not close
     */
    bool saveProject(bool quit = false);

    /*!
     * @param quit whether to show destructive button in dialog
     * @return if quit is true, false if the application should not close
     */
    bool saveProjectAs(bool quit = false);

    void closeEvent(QCloseEvent *event) override;
    void readSettings();
    void saveSettings();
    void setFilename(const QString &fn);
    void addOutput(const QString &msg);
    void addDebugOutput(const QString &msg);
    void refreshOmniBar(const QStringList &flags);

    void addToDockWidgetList(QDockWidget *dockWidget);
    void addDockWidgetAction(QDockWidget *dockWidget, QAction *action);
    void addExtraWidget(QDockWidget *extraDock);


public slots:
    void finalizeOpen();

    void refreshAll();

    void setPanelLock();
    void setTabLocation();

    void on_actionLock_triggered();

    void on_actionLockUnlock_triggered();

    void on_actionTabs_triggered();

    void lockUnlock_Docks(bool what);

    void on_actionRun_Script_triggered();

    void toggleResponsive(bool maybe);

    void openNewFileFailed();

private slots:
    void on_actionAbout_triggered();
    void on_actionExtraGraph_triggered();
    void on_actionExtraHexdump_triggered();
    void on_actionExtraDisassembly_triggered();

    void on_actionRefresh_Panels_triggered();

    void on_actionDisasAdd_comment_triggered();

    void on_actionDefault_triggered();
    void on_actionZen_triggered();

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

    void projectSaved(const QString &name);

    void updateTasksIndicator();

    void mousePressEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event);

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

    Configuration *configuration;

    QList<QDockWidget *> dockWidgets;
    QMap<QAction *, QDockWidget *> dockWidgetActions;
    DisassemblyWidget  *disassemblyDock = nullptr;
    SidebarWidget      *sidebarDock = nullptr;
    HexdumpWidget      *hexdumpDock = nullptr;
    PseudocodeWidget   *pseudocodeDock = nullptr;
    QDockWidget        *graphDock = nullptr;
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
    SdbDock            *sdbDock = nullptr;
    SectionsWidget     *sectionsDock = nullptr;
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
    NewFileDialog      *newFileDialog = nullptr;
#ifdef CUTTER_ENABLE_JUPYTER
    JupyterWidget      *jupyterDock = nullptr;
#endif

    void resetToDefaultLayout();
    void resetToZenLayout();

    void restoreDocks();
    void hideAllDocks();
    void showDefaultDocks();
    void showZenDocks();
    void updateDockActionsChecked();

    void toggleDockWidget(QDockWidget *dock_widget, bool show);

public:
    QString getFilename() const
    {
        return filename;
    }
};

#endif // MAINWINDOW_H
