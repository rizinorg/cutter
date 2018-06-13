#include "DebugToolbar.h"
#include "MainWindow.h"

#include <QAction>
#include <QPainter>
#include <QToolButton>
#include <QMenu>

DebugToolbar::DebugToolbar(MainWindow *main, QWidget *parent) :
    QToolBar(parent),
    main(main)
{
    setObjectName("debugToolbar");
    QIcon startIcon = QIcon(":/img/icons/play_light.svg");
    QIcon stopIcon = QIcon(":/img/icons/media-stop_light.svg");
    QIcon continueIcon = QIcon(":/img/icons/media-skip-forward_light.svg");
    QIcon continueUntilMainIcon = QIcon(":/img/icons/continue_until_main.svg");
    QIcon continueUntilCallIcon = QIcon(":/img/icons/continue_until_call.svg");
    QIcon continueUntilSyscallIcon = QIcon(":/img/icons/continue_until_syscall.svg");
    QIcon stepIcon = QIcon(":/img/icons/step_light.svg");
    QIcon stepOverIcon = QIcon(":/img/icons/step_over_light.svg");

    QAction *actionStart = new QAction(startIcon, tr("Start debug"), parent);
    QAction *actionStop = new QAction(stopIcon, tr("Stop debug"), parent);
    QAction *actionContinue = new QAction(continueIcon, tr("Continue"), parent);
    QAction *actionContinueUntilMain = new QAction(continueUntilMainIcon, tr("Continue until main"), parent);
    QAction *actionContinueUntilCall = new QAction(continueUntilCallIcon, tr("Continue until call"), parent);
    QAction *actionContinueUntilSyscall = new QAction(continueUntilSyscallIcon, tr("Continue until syscall"), parent);
    QAction *actionStep = new QAction(stepIcon, tr("Step"), parent);
    QAction *actionStepOver = new QAction(stepOverIcon, tr("Step over"), parent);

    QToolButton *continueUntilButton = new QToolButton;
    continueUntilButton->setPopupMode(QToolButton::MenuButtonPopup);
    connect(continueUntilButton, &QToolButton::triggered, continueUntilButton, &QToolButton::setDefaultAction);

    QMenu *continueUntilMenu = new QMenu;
    continueUntilMenu->addAction(actionContinueUntilMain);
    continueUntilMenu->addAction(actionContinueUntilCall);
    continueUntilMenu->addAction(actionContinueUntilSyscall);
    continueUntilButton->setMenu(continueUntilMenu);
    continueUntilButton->setDefaultAction(actionContinueUntilMain);

    addAction(actionStart);
    addAction(actionStop);
    addAction(actionContinue);
    addWidget(continueUntilButton);
    addAction(actionStep);
    addAction(actionStepOver);

    connect(actionStop,              &QAction::triggered, Core(), &CutterCore::stopDebug);
    connect(actionStep,              &QAction::triggered, Core(), &CutterCore::stepDebug);
    connect(actionStart,             &QAction::triggered, Core(), &CutterCore::startDebug);
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