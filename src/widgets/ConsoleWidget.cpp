#include <QScrollBar>
#include <QMenu>
#include <QCompleter>
#include <QAction>
#include <QShortcut>
#include <QStringListModel>
#include <QTimer>
#include <iostream>
#include "Cutter.h"
#include "ConsoleWidget.h"
#include "ui_ConsoleWidget.h"
#include "common/Helpers.h"
#include "common/SvgIconEngine.h"


// TODO: Find a way to get to this without copying it here
// source: libr/core/core.c:585..
// remark: u.* is missing
static const QStringList radareArgs( {
    "?", "?v", "whereis", "which", "ls", "rm", "mkdir", "pwd", "cat", "less",
    "dH", "ds", "dso", "dsl", "dc", "dd", "dm", "db ", "db-",
    "dp", "dr", "dcu", "dmd", "dmp", "dml",
    "ec", "ecs", "eco",
    "S", "S.", "S*", "S-", "S=", "Sa", "Sa-", "Sd", "Sl", "SSj", "Sr",
    "s", "s+", "s++", "s-", "s--", "s*", "sa", "sb", "sr",
    "!", "!!",
    "#sha1", "#crc32", "#pcprint", "#sha256", "#sha512", "#md4", "#md5",
    "#!python", "#!perl", "#!vala",
    "V", "v",
    "aa", "ab", "af", "ar", "ag", "at", "a?", "ax", "ad",
    "ae", "aec", "aex", "aep", "aea", "aeA", "aes", "aeso", "aesu", "aesue", "aer", "aei", "aeim", "aef",
    "aaa", "aac", "aae", "aai", "aar", "aan", "aas", "aat", "aap", "aav",
    "af", "afa", "afan", "afc", "afC", "afi", "afb", "afbb", "afn", "afr", "afs", "af*", "afv", "afvn",
    "aga", "agc", "agd", "agl", "agfl",
    // see forbbidenArgs
    //"e", "et", "e-", "e*", "e!", "e?", "env ",
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
    "#!pipe"
});


static const int invalidHistoryPos = -1;



ConsoleWidget::ConsoleWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action),
    ui(new Ui::ConsoleWidget),
    debugOutputEnabled(true),
    maxHistoryEntries(100),
    lastHistoryPosition(invalidHistoryPos)
{
    ui->setupUi(this);

    // Adjust console lineedit
    ui->inputLineEdit->setTextMargins(10, 0, 0, 0);

    setupFont();

    // Adjust text margins of consoleOutputTextEdit
    QTextDocument *console_docu = ui->outputTextEdit->document();
    console_docu->setDocumentMargin(10);

    QAction *actionClear = new QAction(tr("Clear Output"), ui->outputTextEdit);
    connect(actionClear, SIGNAL(triggered(bool)), ui->outputTextEdit, SLOT(clear()));
    actions.append(actionClear);

    // Completion
    QCompleter *completer = new QCompleter(radareArgs, this);
    completer->setMaxVisibleItems(20);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchStartsWith);

    ui->inputLineEdit->setCompleter(completer);

    // Set console output context menu
    ui->outputTextEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->outputTextEdit, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showCustomContextMenu(const QPoint &)));

    // Esc clears inputLineEdit (like OmniBar)
    QShortcut *clear_shortcut = new QShortcut(QKeySequence(Qt::Key_Escape), ui->inputLineEdit);
    connect(clear_shortcut, SIGNAL(activated()), this, SLOT(clear()));
    clear_shortcut->setContext(Qt::WidgetShortcut);

    // Up and down arrows show history
    QShortcut *historyOnUp = new QShortcut(QKeySequence(Qt::Key_Up), ui->inputLineEdit);
    connect(historyOnUp, SIGNAL(activated()), this, SLOT(historyPrev()));
    historyOnUp->setContext(Qt::WidgetShortcut);

    QShortcut *historyOnDown = new QShortcut(QKeySequence(Qt::Key_Down), ui->inputLineEdit);
    connect(historyOnDown, SIGNAL(activated()), this, SLOT(historyNext()));
    historyOnDown->setContext(Qt::WidgetShortcut);

    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(setupFont()));
}

ConsoleWidget::~ConsoleWidget() {}

void ConsoleWidget::setupFont()
{
    ui->outputTextEdit->setFont(Config()->getFont());
}

void ConsoleWidget::addOutput(const QString &msg)
{
    ui->outputTextEdit->appendPlainText(msg);
    scrollOutputToEnd();
}

void ConsoleWidget::addDebugOutput(const QString &msg)
{
    if (debugOutputEnabled) {
        ui->outputTextEdit->appendHtml("<font color=\"red\"> [DEBUG]:\t" + msg + "</font>");
        scrollOutputToEnd();
    }
}

void ConsoleWidget::focusInputLineEdit()
{
    ui->inputLineEdit->setFocus();
}

void ConsoleWidget::removeLastLine()
{
    ui->outputTextEdit->setFocus();
    QTextCursor cur = ui->outputTextEdit->textCursor();
    ui->outputTextEdit->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
    ui->outputTextEdit->moveCursor(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
    ui->outputTextEdit->moveCursor(QTextCursor::End, QTextCursor::KeepAnchor);
    ui->outputTextEdit->textCursor().removeSelectedText();
    ui->outputTextEdit->textCursor().deletePreviousChar();
    ui->outputTextEdit->setTextCursor(cur);
}

void ConsoleWidget::executeCommand(const QString &command)
{
    if (!commandTask.isNull()) {
        return;
    }
    ui->inputLineEdit->setEnabled(false);

    const int originalLines = ui->outputTextEdit->blockCount();
    QTimer *timer = new QTimer(this);
    timer->setInterval(500);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, [this]() {
        ui->outputTextEdit->appendPlainText("Executing the command...");
    });

    QString cmd_line = "<br>[" + RAddressString(Core()->getOffset()) + "]> " + command + "<br>";
    RVA oldOffset = Core()->getOffset();
    commandTask = QSharedPointer<CommandTask>(new CommandTask(command, CommandTask::ColorMode::MODE_256, true));
    connect(commandTask.data(), &CommandTask::finished, this, [this, cmd_line,
          command, originalLines, oldOffset] (const QString & result) {

        if (originalLines < ui->outputTextEdit->blockCount()) {
            removeLastLine();
        }
        ui->outputTextEdit->appendHtml(cmd_line + result);
        scrollOutputToEnd();
        historyAdd(command);
        commandTask = nullptr;
        ui->inputLineEdit->setEnabled(true);
        ui->inputLineEdit->setFocus();
        if (oldOffset != Core()->getOffset()) {
            Core()->updateSeek();
        }
    });
    connect(commandTask.data(), &CommandTask::finished, timer, &QTimer::stop);

    timer->start();
    Core()->getAsyncTaskManager()->start(commandTask);
}

void ConsoleWidget::on_inputLineEdit_returnPressed()
{
    QString input = ui->inputLineEdit->text();
    if (input.isEmpty()) {
        return;
    }
    executeCommand(input);
    ui->inputLineEdit->clear();
}

void ConsoleWidget::on_execButton_clicked()
{
    on_inputLineEdit_returnPressed();
}

void ConsoleWidget::showCustomContextMenu(const QPoint &pt)
{
    QMenu *menu = new QMenu(ui->outputTextEdit);
    menu->addActions(actions);
    menu->exec(ui->outputTextEdit->mapToGlobal(pt));
    menu->deleteLater();
}

void ConsoleWidget::historyNext()
{
    if (!history.isEmpty()) {
        if (lastHistoryPosition > invalidHistoryPos) {
            if (lastHistoryPosition >= history.size()) {
                lastHistoryPosition = history.size() - 1 ;
            }

            --lastHistoryPosition;

            if (lastHistoryPosition >= 0) {
                ui->inputLineEdit->setText(history.at(lastHistoryPosition));
            } else {
                ui->inputLineEdit->clear();
            }


        }
    }
}

void ConsoleWidget::historyPrev()
{
    if (!history.isEmpty()) {
        if (lastHistoryPosition >= history.size() - 1) {
            lastHistoryPosition = history.size() - 2;
        }

        ui->inputLineEdit->setText(history.at(++lastHistoryPosition));
    }
}

void ConsoleWidget::clear()
{
    ui->inputLineEdit->clear();

    invalidateHistoryPosition();

    // Close the potential shown completer popup
    ui->inputLineEdit->clearFocus();
    ui->inputLineEdit->setFocus();
}

void ConsoleWidget::scrollOutputToEnd()
{
    const int maxValue = ui->outputTextEdit->verticalScrollBar()->maximum();
    ui->outputTextEdit->verticalScrollBar()->setValue(maxValue);
}

void ConsoleWidget::historyAdd(const QString &input)
{
    if (history.size() + 1 > maxHistoryEntries) {
        history.removeLast();
    }

    history.prepend(input);

    invalidateHistoryPosition();
}
void ConsoleWidget::invalidateHistoryPosition()
{
    lastHistoryPosition = invalidHistoryPos;
}
