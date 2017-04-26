#ifndef CONSOLEWIDGET_H
#define CONSOLEWIDGET_H

#include <QWidget>

class QRCore;

namespace Ui
{
    class ConsoleWidget;
}

class ConsoleWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ConsoleWidget(QRCore *core, QWidget *parent = 0);
    ~ConsoleWidget();

    void addOutput(const QString &msg);
    void addDebugOutput(const QString &msg);

public slots:
    void focusInputLineEdit();

private slots:
    void on_consoleInputLineEdit_returnPressed();

    void on_consoleExecButton_clicked();

    void showConsoleContextMenu(const QPoint &pt);

    void on_actionClear_ConsoleOutput_triggered();
    void on_actionConsoleSync_with_core_triggered();

    void on_showHistoToolButton_clicked();

private:
    Ui::ConsoleWidget *ui;
    QRCore  *core;
};

#endif // CONSOLEWIDGET_H
