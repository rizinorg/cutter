#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "webserverthread.h"
#include "qrcore.h" // only needed for ut64

#include <QMainWindow>
#include <QList>

class QRCore;
class DockWidget;
class Omnibar;
class MemoryWidget;
class Notepad;
class SideBar;
class Highlighter;
class AsciiHighlighter;
class GraphicsBar;
class FunctionsWidget;
class ImportsWidget;
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

class QDockWidget;

namespace Ui
{
    class MainWindow;
}


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    QRCore *core;
    MemoryWidget    *memoryDock;
    Notepad         *notepadDock;

    bool responsive;

    explicit MainWindow(QWidget *parent = 0, QRCore *kore = nullptr);
    ~MainWindow();

    void start_web_server();
    void closeEvent(QCloseEvent *event);
    void readSettings();
    void setFilename(const QString &fn);
    //void setCore(QRCore *core);
    void seek(const QString &offset, const QString &name = NULL, bool raise_memory_dock = false);
    void seek(const RVA offset, const QString &name = NULL, bool raise_memory_dock = false);
    void updateFrames();
    void refreshFunctions();
    void refreshComments();
    void get_refs(const QString &offset);
    void add_output(QString msg);
    void add_debug_output(QString msg);
    void send_to_notepad(const QString &txt);
    void setWebServerState(bool start);
    void raiseMemoryDock();
    void toggleSideBarTheme();
    void refreshOmniBar(const QStringList &flags);

signals:
    void cursorAddressChanged(RVA address);

public slots:

    void dark();

    void def_theme();

    void on_actionFunctions_triggered();

    void on_actionImports_triggered();

    void on_actionSymbols_triggered();

    void on_actionReloc_triggered();

    void on_actionStrings_triggered();

    void on_actionSections_triggered();

    void on_actionFlags_triggered();

    void on_actionComents_triggered();

    void on_actionNotepad_triggered();

    void on_actionLock_triggered();

    void on_actionLockUnlock_triggered();

    void on_actionTabs_triggered();

    void on_actionhide_bottomPannel_triggered();

    void lockUnlock_Docks(bool what);

    void on_actionDashboard_triggered();

    void on_actionDark_Theme_triggered();

    void on_actionRun_Script_triggered();

    void toggleResponsive(bool maybe);

    void on_backButton_clicked();

private slots:

    void on_actionMem_triggered();

    void on_actionAbout_triggered();

    void on_consoleInputLineEdit_returnPressed();

    void on_showHistoToolButton_clicked();

    void showConsoleContextMenu(const QPoint &pt);

    void on_actionClear_ConsoleOutput_triggered();

    void on_actionRefresh_Panels_triggered();

    void on_actionCalculator_triggered();

    void on_actionCreate_File_triggered();

    void on_actionAssembler_triggered();

    void on_consoleExecButton_clicked();

    void on_actionStart_Web_Server_triggered();

    void on_actionConsoleSync_with_core_triggered();

    void on_actionDisasAdd_comment_triggered();

    void restoreDocks();

    void on_actionDefaut_triggered();

    void hideAllDocks();

    void showDefaultDocks();

    void on_actionFunctionsRename_triggered();

    void on_actionNew_triggered();

    void on_actionSave_triggered();

    void on_actionWhite_Theme_triggered();

    void on_actionSDB_browser_triggered();

    void on_actionLoad_triggered();

    void on_actionShow_Hide_mainsidebar_triggered();

    void on_actionForward_triggered();

    void on_actionTabs_on_Top_triggered();

    void on_actionReset_settings_triggered();

    void webserverThreadFinished();

    void on_actionQuit_triggered();

    void refreshVisibleDockWidgets();

private:
    QDockWidget      *asmDock;
    QDockWidget      *calcDock;
    Omnibar          *omnibar;
    SideBar          *sideBar;

    bool doLock;
    void refreshMem();
    void refreshMem(RVA offset);
    ut64 hexdumpTopOffset;
    ut64 hexdumpBottomOffset;
    QString filename;
    QList<DockWidget *> dockWidgets;
    Ui::MainWindow   *ui;
    Highlighter      *highlighter;
    AsciiHighlighter *hex_highlighter;
    GraphicsBar      *graphicsBar;
    FunctionsWidget  *functionsDock;
    ImportsWidget    *importsDock;
    SymbolsWidget    *symbolsDock;
    RelocsWidget     *relocsDock;
    CommentsWidget   *commentsDock;
    StringsWidget    *stringsDock;
    FlagsWidget      *flagsDock;
    Dashboard        *dashboardDock;
    QLineEdit        *gotoEntry;
    SdbDock          *sdbDock;
    QAction          *sidebar_action;
    SectionsDock     *sectionsDock;
    WebServerThread webserverThread;

    RVA cursor_address;

public:
    RVA getCursorAddress() const        { return cursor_address; }
    void setCursorAddress(RVA addr);
};

#endif // MAINWINDOW_H
