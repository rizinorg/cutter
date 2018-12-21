#pragma once

#include "Cutter.h"

class MainWindow;
class QToolBar;

class DebugActions : public QObject
{
    Q_OBJECT

public:
    explicit DebugActions(QToolBar *toolBar, MainWindow *main);

    void addToToolBar(QToolBar *toolBar);

    QAction *actionStart;
    QAction *actionStartEmul;
    QAction *actionAttach;
    QAction *actionContinue;
    QAction *actionContinueUntilMain;
    QAction *actionContinueUntilCall;
    QAction *actionContinueUntilSyscall;
    QAction *actionStep;
    QAction *actionStepOver;
    QAction *actionStepOut;
    QAction *actionStop;
    QAction *actionAllContinues;

private:
    MainWindow *main;
    QList<QAction *> allActions;

private slots:
    void continueUntilMain();
    void attachProcessDialog();
    void attachProcess(int pid);
    void setAllActionsVisible(bool visible);
};