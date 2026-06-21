#ifndef CONSOLEWIDGET_H
#define CONSOLEWIDGET_H

#include "CutterDockWidget.h"
#include "SearchableDockWidget.h"
#include "common/CommandTask.h"
#include "core/MainWindow.h"

#include <QLocalSocket>
#include <QSocketNotifier>
#include <QStringListModel>

#include <memory>

class QCompleter;
class QShortcut;

namespace Ui {
class ConsoleWidget;
}

/**
 * @brief Widget for console to directly run rizin commands
 */
class ConsoleWidget : public SearchableDockWidget
{
    Q_OBJECT

public:
    explicit ConsoleWidget(MainWindow *main);

    ~ConsoleWidget();

    void setDebugOutputEnabled(bool enabled) { debugOutputEnabled = enabled; }

    void setMaxHistoryEntries(int max) { maxHistoryEntries = max; }

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    QWidget *widgetToFocusOnRaise() override;

    // search related
    void searchChanged(const QString &text, int options) override;
    void findNext() override;
    void findPrev() override;
    void findLast() override;
    void searchBarShown() override;
    void searchBarHidden() override;
    QWidget *searchableArea() const override;

public slots:
    void focusInputLineEdit();

    void addOutput(const QString &msg);
    void addDebugOutput(const QString &msg);

private slots:
    void setupFont();

    void onRzInputLineEditReturnPressed();
    void onDebugeeInputLineEditReturnPressed();
    void onIndexChange();

    void onExecButtonClicked();

    void showCustomContextMenu(const QPoint &pt);

    void historyNext();
    void historyPrev();

    void triggerCompletion();
    void disableCompletion();
    void updateCompletion();

    void clear();

    /**
     * @brief Passes redirected output from the pipe to the terminal and console
     */
    void processQueuedOutput();

private:
    void scrollOutputToEnd();
    void historyAdd(const QString &input);
    void invalidateHistoryPosition();
    void removeLastLine();
    void executeCommand(const QString &command);
    void sendToStdin(const QString &input);
    void setWrap(bool wrap);

    /**
     * @brief Redirects stderr and stdout to the output pipe which is handled by
     *        processQueuedOutput
     */
    void redirectOutput();

    std::shared_ptr<CommandTask> commandTask;

    std::unique_ptr<Ui::ConsoleWidget> ui;
    QAction *actionWrapLines;
    QList<QAction *> actions;
    bool debugOutputEnabled;
    int maxHistoryEntries;
    int lastHistoryPosition;
    QStringList history;
    bool completionActive;
    QStringListModel completionModel;
    QCompleter *completer;
    QShortcut *historyUpShortcut;
    QShortcut *historyDownShortcut;
    FILE *origStderr = nullptr;
    FILE *origStdout = nullptr;
    FILE *origStdin = nullptr;
    QLocalSocket *pipeSocket = nullptr;
    bool hasOutputRedirection = false;
#ifdef Q_OS_WIN
    HANDLE hRead;
    HANDLE hWrite;
#else
    int redirectPipeFds[2];
    int stdinFile = -1;
    QString stdinFifoPath;
    QVector<char> *redirectionBuffer;
#endif
};

#endif // CONSOLEWIDGET_H
