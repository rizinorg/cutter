#include "ConsoleWidget.h"

#include "SearchBarWidget.h"
#include "common/Helpers.h"
#include "core/Cutter.h"
#include "shortcuts/ShortcutManager.h"
#include "ui_ConsoleWidget.h"

#include <QAction>
#include <QCompleter>
#include <QDir>
#include <QMenu>
#include <QScrollBar>
#include <QSettings>
#include <QShortcut>
#include <QStringListModel>
#include <QTimer>
#include <QUuid>

#ifdef Q_OS_WIN
#    include <io.h>
#    include <windows.h>
#    define dup2 _dup2
#    define dup _dup
#    define fileno _fileno
#    define fdopen _fdopen
#    define PIPE_SIZE 65536 // Match Linux size
#    define PIPE_NAME "\\\\.\\pipe\\cutteroutput-%1"
#else
#    include <unistd.h>
#    define PIPE_READ (0)
#    define PIPE_WRITE (1)
#    define STDIN_PIPE_NAME "%1/cutter-stdin-%2"
#endif

enum InputTarget : ut8 { RizinConsole = 0, Debugee = 1 };

static const int invalidHistoryPos = -1;

static const char *consoleWrapSettingsKey = "console.wrap";

ConsoleWidget::ConsoleWidget(MainWindow *main)
    : SearchableDockWidget(main),
      ui(new Ui::ConsoleWidget),
      debugOutputEnabled(true),
      maxHistoryEntries(100),
      lastHistoryPosition(invalidHistoryPos),
      completer(nullptr),
      historyUpShortcut(nullptr),
      historyDownShortcut(nullptr)
{
    ui->setupUi(this);

    // Adjust console lineedit
    ui->rzInputLineEdit->setTextMargins(10, 0, 0, 0);
    ui->debugeeInputLineEdit->setTextMargins(10, 0, 0, 0);

    setupFont();

    // Adjust text margins of consoleOutputTextEdit
    QTextDocument *consoleDocu = ui->outputTextEdit->document();
    consoleDocu->setDocumentMargin(10);

    // Ctrl+` and ';' to toggle console widget
    QAction *toggleConsole = toggleViewAction();
    Shortcuts()->setupAction(*toggleConsole, "Console.toggle");
    connect(toggleConsole, &QAction::triggered, this, [this, toggleConsole]() {
        if (toggleConsole->isChecked()) {
            widgetToFocusOnRaise()->setFocus();
        }
    });

    QAction *actionClear = Shortcuts()->makeAction("Console.clear", this);
    connect(actionClear, &QAction::triggered, this, [this] {
        ui->outputTextEdit->clear();
        ui->outputTextEdit->setExtraSelections({});
        searchBar->clear();
    });
    addAction(actionClear);

    // Ctrl+l to clear the output
    actionClear->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    actions.append(actionClear);

    actionWrapLines = new QAction(tr("Wrap Lines"), ui->outputTextEdit);
    actionWrapLines->setCheckable(true);
    setWrap(QSettings().value(consoleWrapSettingsKey, true).toBool());
    connect(actionWrapLines, &QAction::triggered, this, [this](bool checked) { setWrap(checked); });
    actions.append(actionWrapLines);

    // Completion
    completionActive = false;
    completer = new QCompleter(&completionModel, this);
    completer->setMaxVisibleItems(20);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchStartsWith);
    ui->rzInputLineEdit->setCompleter(completer);

    connect(ui->rzInputLineEdit, &QLineEdit::textEdited, this, &ConsoleWidget::updateCompletion);
    updateCompletion();

    // Set console output context menu
    ui->outputTextEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->outputTextEdit, &QWidget::customContextMenuRequested, this,
            &ConsoleWidget::showCustomContextMenu);

    // Esc clears rzInputLineEdit and debugeeInputLineEdit (like OmniBar)
    QShortcut *rizinClearShortcut =
            Shortcuts()->makeQShortcut("Console.clearRzInputLineEdit", ui->rzInputLineEdit);
    connect(rizinClearShortcut, &QShortcut::activated, this, &ConsoleWidget::clear);
    rizinClearShortcut->setContext(Qt::WidgetShortcut);

    QShortcut *debugeeClearShortcut =
            Shortcuts()->makeQShortcut("Console.clearDebugee", ui->debugeeInputLineEdit);
    connect(debugeeClearShortcut, &QShortcut::activated, this, &ConsoleWidget::clear);
    debugeeClearShortcut->setContext(Qt::WidgetShortcut);

    // Up and down arrows show history
    historyUpShortcut = Shortcuts()->makeQShortcut("Console.historyUp", ui->rzInputLineEdit);
    connect(historyUpShortcut, &QShortcut::activated, this, &ConsoleWidget::historyPrev);
    historyUpShortcut->setContext(Qt::WidgetShortcut);

    historyDownShortcut = Shortcuts()->makeQShortcut("Console.historyDown", ui->rzInputLineEdit);
    connect(historyDownShortcut, &QShortcut::activated, this, &ConsoleWidget::historyNext);
    historyDownShortcut->setContext(Qt::WidgetShortcut);

    const QShortcut *completionShortcut =
            Shortcuts()->makeQShortcut("Console.complete", ui->rzInputLineEdit);
    connect(completionShortcut, &QShortcut::activated, this, &ConsoleWidget::triggerCompletion);

    connect(ui->rzInputLineEdit, &QLineEdit::editingFinished, this,
            &ConsoleWidget::disableCompletion);

    connect(Config(), &Configuration::fontsUpdated, this, &ConsoleWidget::setupFont);

    connect(ui->inputCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &ConsoleWidget::onIndexChange);

    connect(Core(), &CutterCore::debugTaskStateChanged, this, [=, this]() {
        if (Core()->isRedirectableDebugee()) {
            ui->inputCombo->setVisible(true);
        } else {
            ui->inputCombo->setVisible(false);
            // Return to the rizin console
            ui->inputCombo->setCurrentIndex(InputTarget::RizinConsole);
        }
    });

    completer->popup()->installEventFilter(this);
    ui->outputTextEdit->verticalScrollBar()->installEventFilter(this);

    if (Config()->getOutputRedirectionEnabled()) {
        redirectOutput();
    }

    connect(ui->outputTextEdit, &SearchableTextEdit::textChanged, this, [this] {
        if (searchBar && searchBar->isVisible()) {
            const QPair<int, int> range =
                    ui->outputTextEdit->search(searchBar->text(), searchBar->options());
            searchBar->setRange(range.first, range.second);
        }
    });

    connect(ui->outputTextEdit, &SearchableTextEdit::updateRequest, this,
            [this](const QRect &, int dy) {
                if (searchBar && searchBar->isVisible() && dy != 0) {
                    ui->outputTextEdit->highlightMatches();
                }
            });
    connect(ui->rzInputLineEdit, &QLineEdit::returnPressed, this,
            &ConsoleWidget::onRzInputLineEditReturnPressed);
    connect(ui->debugeeInputLineEdit, &QLineEdit::returnPressed, this,
            &ConsoleWidget::onDebugeeInputLineEditReturnPressed);
    connect(ui->execButton, &QToolButton::clicked, this, &ConsoleWidget::onExecButtonClicked);
}

ConsoleWidget::~ConsoleWidget()
{
#ifndef Q_OS_WIN
    if (hasOutputRedirection) {
        ::close(stdinFile);
        remove(stdinFifoPath.toStdString().c_str());
        if (origStdin) {
            dup2(fileno(origStdin), fileno(stdin));
        }
        if (origStderr) {
            dup2(fileno(origStderr), fileno(stderr));
        }
        if (origStdout) {
            dup2(fileno(origStdout), fileno(stdout));
        }
    }
#else

#endif
}

bool ConsoleWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (completer && obj == completer->popup() &&
        // disable up/down shortcuts if completer is shown
        (event->type() == QEvent::Type::Show || event->type() == QEvent::Type::Hide)) {
        const bool enabled = !completer->popup()->isVisible();
        if (historyUpShortcut) {
            historyUpShortcut->setEnabled(enabled);
        }
        if (historyDownShortcut) {
            historyDownShortcut->setEnabled(enabled);
        }
    } else if (searchBar && searchBar->isVisible()
               && obj == ui->outputTextEdit->verticalScrollBar()) {
        if (event->type() == QEvent::Show || event->type() == QEvent::Hide) {
            this->updateSearchBarPosition();
        }
    }
    return false;
}

QWidget *ConsoleWidget::widgetToFocusOnRaise()
{
    return ui->rzInputLineEdit;
}

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
    ui->rzInputLineEdit->setFocus();
}

void ConsoleWidget::removeLastLine()
{
    ui->outputTextEdit->setFocus();
    const QTextCursor cur = ui->outputTextEdit->textCursor();
    ui->outputTextEdit->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
    ui->outputTextEdit->moveCursor(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
    ui->outputTextEdit->moveCursor(QTextCursor::End, QTextCursor::KeepAnchor);
    ui->outputTextEdit->textCursor().removeSelectedText();
    ui->outputTextEdit->textCursor().deletePreviousChar();
    ui->outputTextEdit->setTextCursor(cur);
}

void ConsoleWidget::executeCommand(const QString &command)
{
    if (commandTask) {
        return;
    }
    ui->rzInputLineEdit->setEnabled(false);

    const QString cmdLine = "[" + rzAddressString(Core()->getOffset()) + "]> " + command;
    addOutput(cmdLine);

    const RVA oldOffset = Core()->getOffset();
    commandTask = std::make_shared<CommandTask>(command, CommandTask::ColorMode::MODE_16M);
    connect(commandTask.get(), &CommandTask::finished, this,
            [this, cmdLine, command, oldOffset](const QString &result) {
                ui->outputTextEdit->appendHtml(CutterCore::ansiEscapeToHtml(result));
                scrollOutputToEnd();
                historyAdd(command);
                commandTask.reset();
                ui->rzInputLineEdit->setEnabled(true);
                ui->rzInputLineEdit->setFocus();

                if (oldOffset != Core()->getOffset()) {
                    Core()->updateSeek();
                }
            });

    Core()->getAsyncTaskManager()->start(commandTask);
}

void ConsoleWidget::sendToStdin(const QString &input)
{
#ifndef Q_OS_WIN
    write(stdinFile, (input + "\n").toStdString().c_str(), input.size() + 1);
    fsync(stdinFile);
    addOutput("Sent input: '" + input + "'");
#else
    // Stdin redirection isn't currently available in windows because console applications
    // with stdin already get their own console window with stdin when they are launched
    // that the user can type into.
    addOutput("Unsupported feature");
#endif
}

void ConsoleWidget::onIndexChange()
{
    const int target = ui->inputCombo->currentIndex();
    if (target == InputTarget::Debugee) {
        ui->rzInputLineEdit->setVisible(false);
        ui->debugeeInputLineEdit->setVisible(true);
    } else if (target == InputTarget::RizinConsole) {
        ui->rzInputLineEdit->setVisible(true);
        ui->debugeeInputLineEdit->setVisible(false);
    }
}

void ConsoleWidget::setWrap(bool wrap)
{
    QSettings().setValue(consoleWrapSettingsKey, wrap);
    actionWrapLines->setChecked(wrap);
    ui->outputTextEdit->setLineWrapMode(wrap ? QPlainTextEdit::WidgetWidth
                                             : QPlainTextEdit::NoWrap);
}

void ConsoleWidget::onRzInputLineEditReturnPressed()
{
    const QString input = ui->rzInputLineEdit->text();
    if (input.isEmpty()) {
        return;
    }
    if (input == "clear" || input == "cls") {
        ui->outputTextEdit->clear();
        ui->rzInputLineEdit->clear();
        return;
    }
    executeCommand(input);
    ui->rzInputLineEdit->clear();
}

void ConsoleWidget::onDebugeeInputLineEditReturnPressed()
{
    const QString input = ui->debugeeInputLineEdit->text();
    if (input.isEmpty()) {
        return;
    }
    sendToStdin(input);
    ui->debugeeInputLineEdit->clear();
}

void ConsoleWidget::onExecButtonClicked()
{
    onRzInputLineEditReturnPressed();
}

void ConsoleWidget::showCustomContextMenu(const QPoint &pt)
{
    actionWrapLines->setChecked(ui->outputTextEdit->lineWrapMode() == QPlainTextEdit::WidgetWidth);

    auto *menu = new QMenu(ui->outputTextEdit);
    menu->addActions(actions);
    menu->exec(ui->outputTextEdit->mapToGlobal(pt));
    menu->deleteLater();
}

void ConsoleWidget::historyNext()
{
    if (!history.isEmpty()) {
        if (lastHistoryPosition > invalidHistoryPos) {
            if (lastHistoryPosition >= history.size()) {
                lastHistoryPosition = history.size() - 1;
            }

            --lastHistoryPosition;

            if (lastHistoryPosition >= 0) {
                ui->rzInputLineEdit->setText(history.at(lastHistoryPosition));
            } else {
                ui->rzInputLineEdit->clear();
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

        ui->rzInputLineEdit->setText(history.at(++lastHistoryPosition));
    }
}

void ConsoleWidget::triggerCompletion()
{
    if (completionActive) {
        return;
    }
    completionActive = true;
    updateCompletion();
    completer->complete();
}

void ConsoleWidget::disableCompletion()
{
    if (!completionActive) {
        return;
    }
    completionActive = false;
    updateCompletion();
    completer->popup()->hide();
}

void ConsoleWidget::updateCompletion()
{
    if (!completionActive) {
        completionModel.setStringList({});
        return;
    }

    auto current = ui->rzInputLineEdit->text();
    auto completions = Core()->autocomplete(current, RZ_LINE_PROMPT_DEFAULT);
    const int lastSpace = current.lastIndexOf(' ');
    if (lastSpace >= 0) {
        current = current.left(lastSpace + 1);
        for (auto &s : completions) {
            s = current + s;
        }
    }
    completionModel.setStringList(completions);
}

void ConsoleWidget::clear()
{
    disableCompletion();
    ui->rzInputLineEdit->clear();
    ui->debugeeInputLineEdit->clear();

    invalidateHistoryPosition();

    // Close the potential shown completer popup
    ui->rzInputLineEdit->clearFocus();
    ui->rzInputLineEdit->setFocus();
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

void ConsoleWidget::processQueuedOutput()
{
    // Partial lines are ignored since carriage return is currently unsupported
    while (pipeSocket->canReadLine()) {
        QString output = QString(pipeSocket->readLine());

        if (origStderr) {
            fprintf(origStderr, "%s", output.toStdString().c_str());
        }

        // Get the last segment that wasn't overwritten by carriage return
        output = output.trimmed();
        output = output.remove(0, output.lastIndexOf('\r')).trimmed();
        ui->outputTextEdit->appendHtml(CutterCore::ansiEscapeToHtml(output));
        scrollOutputToEnd();
    }
}

// Haiku doesn't have O_ASYNC
#ifdef Q_OS_HAIKU
#    define O_ASYNC O_NONBLOCK
#endif

void ConsoleWidget::redirectOutput()
{
    // Make sure that we are running in a valid console with initialized output handles
    if (0 > fileno(stderr) && 0 > fileno(stdout)) {
        addOutput("Run cutter in a console to enable rizin output redirection into this widget.");
        return;
    }

    pipeSocket = new QLocalSocket(this);

    origStdin = fdopen(dup(fileno(stdin)), "r");
    origStderr = fdopen(dup(fileno(stderr)), "a");
    origStdout = fdopen(dup(fileno(stdout)), "a");
#ifdef Q_OS_WIN
    QString pipeName = QString::fromLatin1(PIPE_NAME).arg(QUuid::createUuid().toString());

    SECURITY_ATTRIBUTES attributes = { sizeof(SECURITY_ATTRIBUTES), 0, false };
    hWrite =
            CreateNamedPipeW((wchar_t *)pipeName.utf16(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                             PIPE_TYPE_BYTE | PIPE_WAIT, 1, PIPE_SIZE, PIPE_SIZE, 0, &attributes);

    int writeFd = _open_osfhandle((intptr_t)hWrite, _O_WRONLY | _O_TEXT);
    dup2(writeFd, fileno(stdout));
    dup2(writeFd, fileno(stderr));

    pipeSocket->connectToServer(pipeName, QIODevice::ReadOnly);
#else
    pipe(redirectPipeFds);
    stdinFifoPath = QString(STDIN_PIPE_NAME).arg(QDir::tempPath(), QUuid::createUuid().toString());
    mkfifo(stdinFifoPath.toStdString().c_str(), (mode_t)0777);
    stdinFile = open(stdinFifoPath.toStdString().c_str(), O_RDWR | O_ASYNC);

    dup2(stdinFile, fileno(stdin));
    dup2(redirectPipeFds[PIPE_WRITE], fileno(stderr));
    dup2(redirectPipeFds[PIPE_WRITE], fileno(stdout));

    // Attempt to force line buffering to avoid calling processQueuedOutput
    // for partial lines
    setlinebuf(stderr);
    setlinebuf(stdout);

    // Configure the pipe to work in async mode
    fcntl(redirectPipeFds[PIPE_READ], F_SETFL, O_ASYNC | O_NONBLOCK);

    pipeSocket->setSocketDescriptor(redirectPipeFds[PIPE_READ]);
    pipeSocket->connectToServer(QIODevice::ReadOnly);
#endif

    hasOutputRedirection = true;
    connect(pipeSocket, &QIODevice::readyRead, this, &ConsoleWidget::processQueuedOutput);
}

QWidget *ConsoleWidget::searchableArea() const
{
    return ui->outputTextEdit;
}

void ConsoleWidget::searchBarHidden()
{
    ui->outputTextEdit->clearSearch();
}

void ConsoleWidget::searchBarShown()
{
    searchChanged(searchBar->text(), searchBar->options());
}

void ConsoleWidget::findNext()
{
    const int index = ui->outputTextEdit->findNext();
    searchBar->setCurrentIndex(index);
}

void ConsoleWidget::findPrev()
{
    const int index = ui->outputTextEdit->findPrev();
    searchBar->setCurrentIndex(index);
}

void ConsoleWidget::findLast()
{
    const int index = ui->outputTextEdit->findLast();
    searchBar->setCurrentIndex(index);
}

void ConsoleWidget::searchChanged(const QString &text, int options)
{
    const QPair<int, int> range = ui->outputTextEdit->search(text, options);
    searchBar->setRange(range.first, range.second);
}
