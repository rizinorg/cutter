#pragma once

#include <QToolBar>
#include "Cutter.h"

class MainWindow;

class DebugToolbar : public QToolBar
{
    Q_OBJECT

public:
    explicit DebugToolbar(MainWindow *main, QWidget *parent = nullptr);
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