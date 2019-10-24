#include "DebugActions.h"
#include "core/MainWindow.h"
#include "dialogs/AttachProcDialog.h"

#include <QAction>
#include <QPainter>
#include <QMenu>
#include <QList>
#include <QFileInfo>
#include <QToolBar>
#include <QToolButton>

DebugActions::DebugActions(QToolBar *toolBar, MainWindow *main) :
    QObject(main),
    main(main)
{
    setObjectName("DebugActions");
    // setIconSize(QSize(16, 16));

    // define icons
    QIcon startDebugIcon = QIcon(":/img/icons/play_light_debug.svg");
    QIcon startEmulIcon = QIcon(":/img/icons/play_light_emul.svg");
    QIcon startAttachIcon = QIcon(":/img/icons/play_light_attach.svg");
    QIcon stopIcon = QIcon(":/img/icons/media-stop_light.svg");
    QIcon continueIcon = QIcon(":/img/icons/media-skip-forward_light.svg");
    QIcon continueUntilMainIcon = QIcon(":/img/icons/continue_until_main.svg");
    QIcon continueUntilCallIcon = QIcon(":/img/icons/continue_until_call.svg");
    QIcon continueUntilSyscallIcon = QIcon(":/img/icons/continue_until_syscall.svg");
    QIcon stepIcon = QIcon(":/img/icons/step_light.svg");
    QIcon stepOverIcon = QIcon(":/img/icons/step_over_light.svg");
    QIcon stepOutIcon = QIcon(":/img/icons/step_out_light.svg");
    QIcon restartIcon = QIcon(":/img/icons/spin_light.svg");

    // define action labels
    QString startDebugLabel = tr("Start debug");
    QString startEmulLabel = tr("Start emulation");
    QString startAttachLabel = tr("Attach to process");
    QString stopDebugLabel = tr("Stop debug");
    QString stopEmulLabel = tr("Stop emulation");
    QString restartDebugLabel = tr("Restart program");
    QString restartEmulLabel = tr("Restart emulation");
    QString continueLabel = tr("Continue");
    QString continueUMLabel = tr("Continue until main");
    QString continueUCLabel = tr("Continue until call");
    QString continueUSLabel = tr("Continue until syscall");
    QString stepLabel = tr("Step");
    QString stepOverLabel = tr("Step over");
    QString stepOutLabel = tr("Step out");

    // define actions
    actionStart = new QAction(startDebugIcon, startDebugLabel, this);
    actionStart->setShortcut(QKeySequence(Qt::Key_F9));
    actionStartEmul = new QAction(startEmulIcon, startEmulLabel, this);
    actionAttach = new QAction(startAttachIcon, startAttachLabel, this);
    actionStop = new QAction(stopIcon, stopDebugLabel, this);
    actionContinue = new QAction(continueIcon, continueLabel, this);
    actionContinue->setShortcut(QKeySequence(Qt::Key_F5));
    actionContinueUntilMain = new QAction(continueUntilMainIcon, continueUMLabel, this);
    actionContinueUntilCall = new QAction(continueUntilCallIcon, continueUCLabel, this);
    actionContinueUntilSyscall = new QAction(continueUntilSyscallIcon, continueUSLabel, this);
    actionStep = new QAction(stepIcon, stepLabel, this);
    actionStep->setShortcut(QKeySequence(Qt::Key_F7));
    actionStepOver = new QAction(stepOverIcon, stepOverLabel, this);
    actionStepOver->setShortcut(QKeySequence(Qt::Key_F8));
    actionStepOut = new QAction(stepOutIcon, stepOutLabel, this);
    actionStepOut->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_F8));

    QToolButton *startButton = new QToolButton;
    startButton->setPopupMode(QToolButton::MenuButtonPopup);
    connect(startButton, &QToolButton::triggered, startButton, &QToolButton::setDefaultAction);
    QMenu *startMenu = new QMenu(startButton);

    // only emulation is currently allowed
    // startMenu->addAction(actionStart);
    // startMenu->addAction(actionAttach);
    // startButton->setDefaultAction(actionStart);
    startMenu->addAction(actionStartEmul);
    startButton->setDefaultAction(actionStartEmul);
    startButton->setMenu(startMenu);

    QToolButton *continueUntilButton = new QToolButton;
    continueUntilButton->setPopupMode(QToolButton::MenuButtonPopup);
    connect(continueUntilButton, &QToolButton::triggered, continueUntilButton,
            &QToolButton::setDefaultAction);
    QMenu *continueUntilMenu = new QMenu(continueUntilButton);
    continueUntilMenu->addAction(actionContinueUntilMain);
    continueUntilMenu->addAction(actionContinueUntilCall);
    continueUntilMenu->addAction(actionContinueUntilSyscall);
    continueUntilButton->setMenu(continueUntilMenu);
    continueUntilButton->setDefaultAction(actionContinueUntilMain);

    // define toolbar widgets and actions
    toolBar->addWidget(startButton);
    toolBar->addAction(actionContinue);
    toolBar->addAction(actionStop);
    actionAllContinues = toolBar->addWidget(continueUntilButton);
    toolBar->addAction(actionStep);
    toolBar->addAction(actionStepOver);
    toolBar->addAction(actionStepOut);

    allActions = {actionStop, actionAllContinues, actionContinue, actionContinueUntilCall, actionContinueUntilMain, actionContinueUntilSyscall, actionStep, actionStepOut, actionStepOver};
    // hide allactions
    setAllActionsVisible(false);

    connect(actionStop, &QAction::triggered, Core(), &CutterCore::stopDebug);
    connect(actionStop, &QAction::triggered, [ = ]() {
        actionStart->setVisible(true);
        actionStartEmul->setVisible(true);
        actionAttach->setVisible(true);
        actionStop->setText(stopDebugLabel);
        actionStart->setText(startDebugLabel);
        actionStart->setIcon(startDebugIcon);
        actionStartEmul->setText(startEmulLabel);
        actionStartEmul->setIcon(startEmulIcon);
        setAllActionsVisible(false);
    });
    connect(actionStep, &QAction::triggered, Core(), &CutterCore::stepDebug);
    connect(actionStart, &QAction::triggered, [ = ]() {
        // check if file is executable before starting debug
        QString filename = Core()->getConfig("file.path").section(QLatin1Char(' '), 0, 0);
        QFileInfo info(filename);
        if (!Core()->currentlyDebugging && !info.isExecutable()) {
            QMessageBox msgBox;
            msgBox.setText(tr("File '%1' does not have executable permissions.").arg(filename));
            msgBox.exec();
            return;
        }
        setAllActionsVisible(true);
        actionAttach->setVisible(false);
        actionStartEmul->setVisible(false);
        actionStart->setText(restartDebugLabel);
        actionStart->setIcon(restartIcon);
        Core()->startDebug();
    });

    connect(actionAttach, &QAction::triggered, this, &DebugActions::attachProcessDialog);
    connect(actionStartEmul, &QAction::triggered, Core(), &CutterCore::startEmulation);
    connect(actionStartEmul, &QAction::triggered, [ = ]() {
        setAllActionsVisible(true);
        actionStart->setVisible(false);
        actionAttach->setVisible(false);
        actionContinueUntilMain->setVisible(false);
        actionStepOut->setVisible(false);
        continueUntilButton->setDefaultAction(actionContinueUntilSyscall);
        actionStartEmul->setText(restartEmulLabel);
        actionStartEmul->setIcon(restartIcon);
        actionStop->setText(stopEmulLabel);
    });
    connect(actionStepOver, &QAction::triggered, Core(), &CutterCore::stepOverDebug);
    connect(actionStepOut, &QAction::triggered, Core(), &CutterCore::stepOutDebug);
    connect(actionContinue, &QAction::triggered, Core(), &CutterCore::continueDebug);
    connect(actionContinueUntilMain, &QAction::triggered, this, &DebugActions::continueUntilMain);
    connect(actionContinueUntilCall, &QAction::triggered, Core(), &CutterCore::continueUntilCall);
    connect(actionContinueUntilSyscall, &QAction::triggered, Core(), &CutterCore::continueUntilSyscall);
}

void DebugActions::continueUntilMain()
{
    Core()->continueUntilDebug("main");
}

void DebugActions::attachProcessDialog()
{
    AttachProcDialog dialog(main);
    bool success = false;
    while (!success) {
        success = true;
        if (dialog.exec()) {
            int pid = dialog.getPID();
            if (pid >= 0) {
                attachProcess(pid);
            } else {
                success = false;
                QMessageBox msgBox;
                msgBox.setText(tr("Error attaching. No process selected!"));
                msgBox.exec();
            }
        }
    }
}

void DebugActions::attachProcess(int pid)
{
    QString stopAttachLabel = tr("Detach from process");
    // hide unwanted buttons
    setAllActionsVisible(true);
    actionStart->setVisible(false);
    actionStartEmul->setVisible(false);
    actionStop->setText(stopAttachLabel);
    // attach
    Core()->attachDebug(pid);
}

void DebugActions::setAllActionsVisible(bool visible)
{
    for (QAction *action : allActions) {
        action->setVisible(visible);
    }
}
