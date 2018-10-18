#ifndef CONSOLEWIDGET_H
#define CONSOLEWIDGET_H

#include <memory>
#include "MainWindow.h"
#include "CutterDockWidget.h"
#include "common/CommandTask.h"

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

public slots:
    void focusInputLineEdit();

    void addOutput(const QString &msg);
    void addDebugOutput(const QString &msg);

private slots:
    void setupFont();

    void on_inputLineEdit_returnPressed();

    void on_execButton_clicked();

    void showCustomContextMenu(const QPoint &pt);

    void historyNext();
    void historyPrev();

    void clear();

private:
    void scrollOutputToEnd();
    void historyAdd(const QString &input);
    void invalidateHistoryPosition();
    void removeLastLine();
    void executeCommand(const QString &command);

    QSharedPointer<CommandTask> commandTask;

    std::unique_ptr<Ui::ConsoleWidget> ui;
    QList<QAction *> actions;
    bool debugOutputEnabled;
    int maxHistoryEntries;
    int lastHistoryPosition;
    QStringList history;
};

#endif // CONSOLEWIDGET_H
