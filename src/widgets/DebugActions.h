#pragma once

#include "core/Cutter.h"
#include "dialogs/RemoteDebugDialog.h"

#include <QAction>

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
    QAction *actionStartRemote;
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

    // Continue/suspend and start/restart interchange during runtime
    QIcon continueIcon;
    QIcon suspendIcon;
    QIcon restartIcon;
    QIcon startDebugIcon;
    QString suspendLabel;
    QString continueLabel;
    QString restartDebugLabel;
    QString startDebugLabel;

    // Stop and Detach interchange during runtime
    QIcon detachIcon;
    QIcon stopIcon;
    
private:
    /**
     * @brief buttons that will be disabled/enabled on (disable/enable)DebugToolbar
     */
    QList<QAction *> toggleActions;
    QList<QAction *> toggleConnectionActions;
    QList<QAction *> allActions;
    QToolButton *continueUntilButton;
    RemoteDebugDialog *remoteDialog = nullptr;
    MainWindow *main;
    bool acceptedDebugWarning = false;

    // TODO: Remove once debug is stable
    void showDebugWarning();

private slots:
    void continueUntilMain();
    void startDebug();
    void attachProcessDialog();
    void attachProcess(int pid);
    void attachRemoteDialog();
    void attachRemoteDebugger();
    void onAttachedRemoteDebugger(bool successfully);
    void setAllActionsVisible(bool visible);
    void setButtonVisibleIfMainExists();
    void chooseThemeIcons();
};
