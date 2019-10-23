#pragma once

#include "core/Cutter.h"

class MainWindow;
class QToolBar;
class QToolButton;

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
    QAction *actionSuspend;
    QAction *actionAllContinues;

private:
    /**
     * @brief buttons that will be disabled/enabled on (disable/enable)DebugToolbar
     */
    QList<QAction *> toggleActions;
    MainWindow *main;
    QList<QAction *> allActions;
    QToolButton *continueUntilButton;

private slots:
    void continueUntilMain();
    void attachProcessDialog();
    void attachProcess(int pid);
    void setAllActionsVisible(bool visible);
    void setButtonVisibleIfMainExists();
};
