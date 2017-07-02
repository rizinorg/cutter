#ifndef CONSOLEWIDGET_H
#define CONSOLEWIDGET_H

#include <QWidget>

class IaitoRCore;

class QAction;

namespace Ui
{
    class ConsoleWidget;
}


class ConsoleWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ConsoleWidget(IaitoRCore *core, QWidget *parent = 0);
    ~ConsoleWidget();

    void addOutput(const QString &msg);
    void addDebugOutput(const QString &msg);

    void setDebugOutputEnabled(bool enabled) { debugOutputEnabled = enabled; }

    void setMaxHistoryEntries(int max) { maxHistoryEntries = max; }

public slots:
    void focusInputLineEdit();

private slots:
    void on_inputLineEdit_returnPressed();

    void on_execButton_clicked();

    void showCustomContextMenu(const QPoint &pt);

    void syncWithCoreToggled(bool checked);

    void historyNext();
    void historyPrev();

    void clear();

private:
    void scrollOutputToEnd();
    void historyAdd(const QString &input);
    void invalidateHistoryPosition();

    Ui::ConsoleWidget *ui;
    IaitoRCore  *core;
    QList<QAction *> actions;
    bool debugOutputEnabled;
    int maxHistoryEntries;
    int lastHistoryPosition;
    QStringList history;
};

#endif // CONSOLEWIDGET_H
