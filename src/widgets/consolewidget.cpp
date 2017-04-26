#include "consolewidget.h"
#include "ui_consolewidget.h"

#include "helpers.h"
#include "qrcore.h"

#include <QScrollBar>
#include <QMenu>
#include <QCompleter>
//TODO: delete
#include <QStringListModel>


// TODO: Find a way to get to this without copying it here
// source: libr/core/core.c:585..
#define CMDS (sizeof (radare_argv)/sizeof(const char*))
static const char *radare_argv[] = {
    "?", "?v", "whereis", "which", "ls", "rm", "mkdir", "pwd", "cat", "less",
    "dH", "ds", "dso", "dsl", "dc", "dd", "dm", "db ", "db-",
        "dp", "dr", "dcu", "dmd", "dmp", "dml",
    "ec","ecs", "eco",
    "S", "S.", "S*", "S-", "S=", "Sa", "Sa-", "Sd", "Sl", "SSj", "Sr",
    "s", "s+", "s++", "s-", "s--", "s*", "sa", "sb", "sr",
    "!", "!!",
    "#sha1", "#crc32", "#pcprint", "#sha256", "#sha512", "#md4", "#md5",
    "#!python", "#!perl", "#!vala",
    "V", "v",
    "aa", "ab", "af", "ar", "ag", "at", "a?", "ax", "ad",
    "ae", "aec", "aex", "aep", "aea", "aeA", "aes", "aeso", "aesu", "aesue", "aer", "aei", "aeim", "aef",
    "aaa", "aac","aae", "aai", "aar", "aan", "aas", "aat", "aap", "aav",
    "af", "afa", "afan", "afc", "afC", "afi", "afb", "afbb", "afn", "afr", "afs", "af*", "afv", "afvn",
    "aga", "agc", "agd", "agl", "agfl",
    "e", "et", "e-", "e*", "e!", "e?", "env ",
    "i", "ii", "iI", "is", "iS", "iz",
    "q", "q!",
    "f", "fl", "fr", "f-", "f*", "fs", "fS", "fr", "fo", "f?",
    "m", "m*", "ml", "m-", "my", "mg", "md", "mp", "m?",
    "o", "o+", "oc", "on", "op", "o-", "x", "wf", "wF", "wta", "wtf", "wp",
    "t", "to", "t-", "tf", "td", "td-", "tb", "tn", "te", "tl", "tk", "ts", "tu",
    "(", "(*", "(-", "()", ".", ".!", ".(", "./",
    "r", "r+", "r-",
    "b", "bf", "b?",
    "/", "//", "/a", "/c", "/h", "/m", "/x", "/v", "/v2", "/v4", "/v8", "/r", "/re",
    "y", "yy", "y?",
    "wx", "ww", "w?", "wxf",
    "p6d", "p6e", "p8", "pb", "pc",
    "pd", "pda", "pdb", "pdc", "pdj", "pdr", "pdf", "pdi", "pdl", "pds", "pdt",
    "pD", "px", "pX", "po", "pf", "pf.", "pf*", "pf*.", "pfd", "pfd.", "pv", "p=", "p-",
    "pfj", "pfj.", "pfv", "pfv.",
    "pm", "pr", "pt", "ptd", "ptn", "pt?", "ps", "pz", "pu", "pU", "p?",
    "z", "z*", "zj", "z-", "z-*",
    "za", "zaf", "zaF",
    "zo", "zoz", "zos",
    "zfd", "zfs", "zfz",
    "z/", "z/*",
    "zc",
    "zs", "zs+", "zs-", "zs-*", "zsr",
    "#!pipe",
    NULL
};


ConsoleWidget::ConsoleWidget(QRCore *core, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ConsoleWidget),
    core(core)
{
    ui->setupUi(this);

    // Adjust console lineedit
    ui->consoleInputLineEdit->setTextMargins(10, 0, 0, 0);

    /*
    ui->consoleOutputTextEdit->setFont(QFont("Monospace", 8));
    ui->consoleOutputTextEdit->setStyleSheet("background-color:black;color:gray;");
    ui->consoleInputLineEdit->setStyleSheet("background-color:black;color:gray;");
    */

    // Adjust text margins of consoleOutputTextEdit
    QTextDocument *console_docu = ui->consoleOutputTextEdit->document();
    console_docu->setDocumentMargin(10);

    // Fix output panel font
    qhelpers::normalizeFont(ui->consoleOutputTextEdit);

    // Set console output context menu
    ui->consoleOutputTextEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->consoleOutputTextEdit, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showConsoleContextMenu(const QPoint &)));
}

ConsoleWidget::~ConsoleWidget()
{
    delete ui;
}

void ConsoleWidget::addOutput(const QString &msg)
{
    ui->consoleOutputTextEdit->appendPlainText(msg);
    ui->consoleOutputTextEdit->verticalScrollBar()->setValue(ui->consoleOutputTextEdit->verticalScrollBar()->maximum());
}

void ConsoleWidget::addDebugOutput(const QString &msg)
{
    ui->consoleOutputTextEdit->appendHtml("<font color=\"red\"> [DEBUG]:\t" + msg + "</font>");
    ui->consoleOutputTextEdit->verticalScrollBar()->setValue(ui->consoleOutputTextEdit->verticalScrollBar()->maximum());
}

void ConsoleWidget::focusInputLineEdit()
{
    ui->consoleInputLineEdit->setFocus();
}

void ConsoleWidget::on_consoleInputLineEdit_returnPressed()
{
    if (this->core)
    {
        QString input = ui->consoleInputLineEdit->text();
        ui->consoleOutputTextEdit->appendPlainText(this->core->cmd(input));
        ui->consoleOutputTextEdit->verticalScrollBar()->setValue(ui->consoleOutputTextEdit->verticalScrollBar()->maximum());
        // Add new command to history
        QCompleter *completer = ui->consoleInputLineEdit->completer();
        if (completer != NULL)
        {
            QStringListModel *completerModel = (QStringListModel *)(completer->model());
            if (completerModel != NULL)
                completerModel->setStringList(completerModel->stringList() << input);
        }

        ui->consoleInputLineEdit->setText("");
    }
}

void ConsoleWidget::on_consoleExecButton_clicked()
{
    on_consoleInputLineEdit_returnPressed();
}

void ConsoleWidget::showConsoleContextMenu(const QPoint &pt)
{
    // Set console output popup menu
    QMenu *menu = ui->consoleOutputTextEdit->createStandardContextMenu();
    menu->clear();
// TODO:
//    menu->addAction(ui->actionClear_ConsoleOutput);
//    menu->addAction(ui->actionConsoleSync_with_core);
    //ui->actionConsoleSync_with_core->setChecked(true);
    ui->consoleOutputTextEdit->setContextMenuPolicy(Qt::CustomContextMenu);

    menu->exec(ui->consoleOutputTextEdit->mapToGlobal(pt));
    delete menu;
}

void ConsoleWidget::on_actionConsoleSync_with_core_triggered()
{
// TODO:
//    if (ui->actionConsoleSync_with_core->isChecked())
//    {
//        //Enable core syncronization
//    }
//    else
//    {
//        // Disable core sync
//    }
}

void ConsoleWidget::on_actionClear_ConsoleOutput_triggered()
{
    ui->consoleOutputTextEdit->clear();
}


void ConsoleWidget::on_showHistoToolButton_clicked()
{
    QCompleter *completer = ui->consoleInputLineEdit->completer();
    if (completer == NULL)
        return;

    if (ui->showHistoToolButton->isChecked())
    {
        completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
        // Uhm... shouldn't it be called always?
        completer->complete();
    }
    else
    {
        completer->setCompletionMode(QCompleter::PopupCompletion);
    }
}
