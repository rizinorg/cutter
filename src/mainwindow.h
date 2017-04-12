#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QComboBox>
#include <QTreeWidgetItem>
#include <QDockWidget>
#include <QLineEdit>
#include <QSettings>
#include <QList>

#include "highlighter.h"
#include "hexascii_highlighter.h"
#include "helpers.h"
#include "qrcore.h"

#include "widgets/memorywidget.h"
#include "widgets/functionswidget.h"
#include "widgets/sectionswidget.h"
#include "widgets/commentswidget.h"
#include "widgets/importswidget.h"
#include "widgets/symbolswidget.h"
#include "widgets/stringswidget.h"
#include "widgets/sectionsdock.h"
#include "widgets/relocswidget.h"
#include "widgets/flagswidget.h"
#include "widgets/codegraphic.h"
#include "widgets/dashboard.h"
#include "widgets/notepad.h"
#include "widgets/sidebar.h"
#include "widgets/sdbdock.h"
#include "widgets/omnibar.h"

#include "webserverthread.h"
#include "newfiledialog.h"
#include "helpers.h"

namespace Ui
{
    class MainWindow;
}


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    QRCore *core;
    QDockWidget      *asmDock;
    QDockWidget      *calcDock;
    Omnibar          *omnibar;
    MemoryWidget     *memoryDock;
    Notepad          *notepadDock;
    SideBar          *sideBar;

    bool responsive;
    QString current_address;

    explicit MainWindow(QWidget *parent = 0, QRCore *kore = nullptr);
    ~MainWindow();

    void start_web_server();
    void closeEvent(QCloseEvent *event);
    void readSettings();
    void setFilename(QString fn);
    void setCore(QRCore *core);
    void seek(const QString &offset, const QString &name = NULL);
    void updateFrames();
    void refreshFunctions();
    void refreshComments();
    void refreshFlags();
    void get_refs(const QString &offset);
    void add_output(QString msg);
    void add_debug_output(QString msg);
    void send_to_notepad(QString txt);
    void adjustColumns(QTreeWidget *tw);
    void appendRow(QTreeWidget *tw, const QString &str, const QString &str2 = NULL,
                   const QString &str3 = NULL, const QString &str4 = NULL, const QString &str5 = NULL);

    void setWebServerState(bool start);

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

    void hideDummyColumns();

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

private:
    void refreshFlagspaces();
    bool doLock;
    void refreshMem(QString off);
    void setup_mem();
    ut64 hexdumpTopOffset;
    ut64 hexdumpBottomOffset;
    QString filename;
    QList<QDockWidget *> dockList;
    QLabel           *logo;
    Ui::MainWindow   *ui;
    Highlighter      *highlighter;
    Highlighter      *highlighter_5;
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
};

#endif // MAINWINDOW_H
