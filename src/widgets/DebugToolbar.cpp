#include "DebugToolbar.h"
#include "MainWindow.h"
#include "dialogs/AttachProcDialog.h"

#include <QAction>
#include <QPainter>
#include <QToolButton>
#include <QMenu>

DebugToolbar::DebugToolbar(MainWindow *main, QWidget *parent) :
    QToolBar(parent),
    main(main)
{
    setObjectName("debugToolbar");
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

    actionStart = new QAction(startDebugIcon, tr("Start debug"), parent);
    actionStartEmul = new QAction(startEmulIcon, tr("Start emulation"), parent);
    QAction *actionAttach = new QAction(startAttachIcon, tr("Attach to process"), parent);
    QAction *actionStop = new QAction(stopIcon, tr("Stop debug"), parent);
    QAction *actionContinue = new QAction(continueIcon, tr("Continue"), parent);
    QAction *actionContinueUntilMain = new QAction(continueUntilMainIcon, tr("Continue until main"), parent);
    QAction *actionContinueUntilCall = new QAction(continueUntilCallIcon, tr("Continue until call"), parent);
    QAction *actionContinueUntilSyscall = new QAction(continueUntilSyscallIcon, tr("Continue until syscall"), parent);
    QAction *actionStep = new QAction(stepIcon, tr("Step"), parent);
    QAction *actionStepOver = new QAction(stepOverIcon, tr("Step over"), parent);

    QToolButton *startButton = new QToolButton;
    startButton->setPopupMode(QToolButton::MenuButtonPopup);
    connect(startButton, &QToolButton::triggered, startButton, &QToolButton::setDefaultAction);
    QMenu *startMenu = new QMenu;
    startMenu->addAction(actionStart);
    startMenu->addAction(actionStartEmul);
    startMenu->addAction(actionAttach);
    startButton->setDefaultAction(actionStart);
    startButton->setMenu(startMenu);

    QToolButton *continueUntilButton = new QToolButton;
    continueUntilButton->setPopupMode(QToolButton::MenuButtonPopup);
    connect(continueUntilButton, &QToolButton::triggered, continueUntilButton, &QToolButton::setDefaultAction);
    QMenu *continueUntilMenu = new QMenu;
    continueUntilMenu->addAction(actionContinueUntilMain);
    continueUntilMenu->addAction(actionContinueUntilCall);
    continueUntilMenu->addAction(actionContinueUntilSyscall);
    continueUntilButton->setMenu(continueUntilMenu);
    continueUntilButton->setDefaultAction(actionContinueUntilMain);

    addWidget(startButton);
    addAction(actionStop);
    addAction(actionContinue);
    addWidget(continueUntilButton);
    addAction(actionStep);
    addAction(actionStepOver);

    connect(actionStop,              &QAction::triggered, Core(), &CutterCore::stopDebug);
    connect(actionStop,              &QAction::triggered, [=](){
                                                                actionContinue->setVisible(true);
                                                                actionStart->setVisible(true);
                                                                actionStartEmul->setVisible(true);
                                                                actionAttach->setVisible(true);
                                                                actionContinueUntilMain->setVisible(true);
                                                                actionContinueUntilCall->setVisible(true);
                                                                this->colorToolbar(false);
                                                                });
    connect(actionStep,              &QAction::triggered, Core(), &CutterCore::stepDebug);
    connect(actionStart,             &QAction::triggered, Core(), &CutterCore::startDebug);
    connect(actionStart,             &QAction::triggered, [=](){
                                                                this->colorToolbar(true);
                                                                actionAttach->setVisible(false);
                                                                actionStartEmul->setVisible(false);
                                                                });
    connect(actionAttach,           &QAction::triggered, this,   &DebugToolbar::attachProcessDialog);
    connect(actionStartEmul,        &QAction::triggered, Core(), &CutterCore::startEmulation);
    connect(actionStartEmul,        &QAction::triggered, [=](){
                                                                actionContinue->setVisible(false);
                                                                actionStart->setVisible(false);
                                                                actionAttach->setVisible(false);
                                                                actionContinueUntilMain->setVisible(false);
                                                                actionContinueUntilCall->setVisible(false);
                                                                continueUntilButton->setDefaultAction(actionContinueUntilSyscall);
                                                                this->colorToolbar(true);
                                                                });
    connect(actionStepOver,          &QAction::triggered, Core(), &CutterCore::stepOverDebug);
    connect(actionContinue,          &QAction::triggered, Core(), &CutterCore::continueDebug);
    connect(actionContinueUntilMain, &QAction::triggered, this,   &DebugToolbar::continueUntilMain);
    connect(actionContinueUntilCall, &QAction::triggered, Core(), &CutterCore::continueUntilCall);
    connect(actionContinueUntilSyscall, &QAction::triggered, Core(),&CutterCore::continueUntilSyscall);
}

void DebugToolbar::continueUntilMain()
{
    Core()->continueUntilDebug(tr("main"));
}

void DebugToolbar::colorToolbar(bool p)
{
    if (p) {
        this->setStyleSheet("QToolBar {background: green;}");
    } else {
        this->setStyleSheet("");
    }
}

void DebugToolbar::attachProcessDialog()
{
    AttachProcDialog *dialog = new AttachProcDialog(this);

    if (dialog->exec()) {
        int pid = dialog->getPID();
        attachProcess(pid);
    }
}

void DebugToolbar::attachProcess(int pid)
{
    // hide unwanted buttons
    this->colorToolbar(true);
    this->actionStart->setVisible(false);
    this->actionStartEmul->setVisible(false);
    // attach
    Core()->attachDebug(pid);
}