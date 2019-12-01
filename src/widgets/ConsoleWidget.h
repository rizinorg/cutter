#ifndef CONSOLEWIDGET_H
#define CONSOLEWIDGET_H

#include "core/MainWindow.h"
#include "CutterDockWidget.h"
#include "common/CommandTask.h"
#include "common/DirectionalComboBox.h"

#include <QStringListModel>
#include <QSocketNotifier>
#include <QLocalSocket>

#include <memory>

class QCompleter;
class QShortcut;

namespace Ui {
class ConsoleWidget;
}

class ConsoleWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit ConsoleWidget(MainWindow *main, QAction *action = nullptr);

    ~ConsoleWidget();

    void setDebugOutputEnabled(bool enabled)
    {
        debugOutputEnabled = enabled;
    }

    void setMaxHistoryEntries(int max)
    {
        maxHistoryEntries = max;
    }

protected:
    bool eventFilter(QObject *obj, QEvent *event);

public slots:
    void focusInputLineEdit();

    void addOutput(const QString &msg);
    void addDebugOutput(const QString &msg);

private slots:
    void setupFont();

    void on_r2InputLineEdit_returnPressed();
    void on_debugeeInputLineEdit_returnPressed();
    void onIndexChange();

    void on_execButton_clicked();

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

    QSharedPointer<CommandTask> commandTask;

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
    FILE *origStderr;
    FILE *origStdout;
    FILE *origStdin;
    QLocalSocket *pipeSocket;
#ifdef Q_OS_WIN
    HANDLE hRead;
    HANDLE hWrite;
#else
    int redirectPipeFds[2];
    int stdinFile;
    QString stdinFifoPath;
    QVector<char> *redirectionBuffer;
    QSocketNotifier *outputNotifier;
#endif
};

#endif // CONSOLEWIDGET_H
