#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>

#include "cutter.h" // only needed for ut64
#include "widgets/DisassemblyWidget.h"
#include "widgets/SidebarWidget.h"
#include "widgets/HexdumpWidget.h"
#include "widgets/PseudocodeWidget.h"
#include "utils/Configuration.h"

#include <QMainWindow>
#include <QList>

class CutterCore;
class Omnibar;
class PreviewWidget;
class Notepad;
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
class SectionsDock;
class ConsoleWidget;
class EntrypointWidget;
class DisassemblerGraphView;
class ClassesWidget;
class ResourcesWidget;

class QDockWidget;

namespace Ui
{
    class MainWindow;
}


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    bool responsive;

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void openNewFile(const QString &fn, int analLevel = -1, QList<QString> advancedOptions = QList<QString>());
    void displayNewFileDialog();
    void displayAnalysisOptionsDialog(int analLevel, QList<QString> advancedOptions);
    void openProject(const QString &project_name);

    void initUI();
    void finalizeOpen();

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
    void sendToNotepad(const QString &txt);
    void refreshOmniBar(const QStringList &flags);

public slots:

    void refreshAll();

    void setPanelLock();
    void setTabLocation();

    void on_actionLock_triggered();

    void on_actionLockUnlock_triggered();

    void on_actionTabs_triggered();

    void lockUnlock_Docks(bool what);

    void on_actionRun_Script_triggered();

    void toggleResponsive(bool maybe);

    void backButton_clicked();

private slots:
    void on_actionAbout_triggered();

    void on_actionRefresh_Panels_triggered();

    void on_actionDisasAdd_comment_triggered();

    void on_actionDefault_triggered();

    void on_actionFunctionsRename_triggered();

    void on_actionNew_triggered();

    void on_actionSave_triggered();
    void on_actionSaveAs_triggered();

    void on_actionUndoSeek_triggered();
    void on_actionRedoSeek_triggered();

    void on_actionOpen_triggered();

    void on_actionForward_triggered();

    void on_actionTabs_on_Top_triggered();

    void on_actionReset_settings_triggered();

    void on_actionQuit_triggered();

    void on_actionRefresh_contents_triggered();

    void on_actionPreferences_triggered();

    void on_actionAnalyze_triggered();

    void on_actionImportPDB_triggered();

    void projectSaved(const QString &name);

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
    Configuration *configuration;

    QList<QDockWidget *> dockWidgets;
    QMap<QAction *, QDockWidget *> dockWidgetActions;
    Notepad            *notepadDock = nullptr;
    DisassemblyWidget  *disassemblyDock = nullptr;
    SidebarWidget      *sidebarDock = nullptr;
    HexdumpWidget      *hexdumpDock = nullptr;
    PseudocodeWidget   *pseudocodeDock = nullptr;
    QDockWidget        *graphDock = nullptr;
    EntrypointWidget   *entrypointDock = nullptr;
    FunctionsWidget    *functionsDock = nullptr;
    ImportsWidget      *importsDock = nullptr;
    ExportsWidget      *exportsDock = nullptr;
    SymbolsWidget      *symbolsDock = nullptr;
    RelocsWidget       *relocsDock = nullptr;
    CommentsWidget     *commentsDock = nullptr;
    StringsWidget      *stringsDock = nullptr;
    FlagsWidget        *flagsDock = nullptr;
    Dashboard          *dashboardDock = nullptr;
    QLineEdit          *gotoEntry = nullptr;
    SdbDock            *sdbDock = nullptr;
    SectionsDock       *sectionsDock = nullptr;
    ConsoleWidget      *consoleDock = nullptr;
    ClassesWidget      *classesDock = nullptr;
    ResourcesWidget    *resourcesDock = nullptr;
    DisassemblerGraphView *graphView = nullptr;
    QDockWidget        *asmDock = nullptr;
    QDockWidget        *calcDock = nullptr;

    void toggleDockWidget(QDockWidget *dock_widget, bool show);

    void resetToDefaultLayout();

    void restoreDocks();
    void hideAllDocks();
    void showDefaultDocks();
    void updateDockActionsChecked();

public:
    QString getFilename() const         { return filename; }
};

#endif // MAINWINDOW_H
