#ifndef CONSOLEWIDGET_H
#define CONSOLEWIDGET_H

#include <QWidget>
#include <memory>
#include "MainWindow.h"

namespace Ui
{
    class ConsoleWidget;
}


class ConsoleWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit ConsoleWidget(const QString &title, QWidget *parent = nullptr, Qt::WindowFlags flags = 0);
    explicit ConsoleWidget(QWidget *parent = nullptr, Qt::WindowFlags flags = 0);

    ~ConsoleWidget();

    void addOutput(const QString &msg);
    void addDebugOutput(const QString &msg);

    void setDebugOutputEnabled(bool enabled) { debugOutputEnabled = enabled; }

    void setMaxHistoryEntries(int max) { maxHistoryEntries = max; }

public slots:
    void focusInputLineEdit();

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
    QString executeCommand(QString command);

    std::unique_ptr<Ui::ConsoleWidget> ui;
    QList<QAction *> actions;
    bool debugOutputEnabled;
    int maxHistoryEntries;
    int lastHistoryPosition;
    QStringList history;
};

#endif // CONSOLEWIDGET_H
