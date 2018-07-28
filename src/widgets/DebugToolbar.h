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

private:
    MainWindow *main;

private slots:
    void continueUntilMain();
    void colorToolbar(bool p);
    void attachProcessDialog();
    void attachProcess(int pid);
};