#include "DebugToolbar.h"
#include "MainWindow.h"

#include <QAction>
#include <QPainter>

DebugToolbar::DebugToolbar(MainWindow *main, QWidget *parent) :
    QToolBar(parent),
    main(main)
{
    setObjectName("debugToolbar");
    QIcon startIcon = QIcon(":/img/icons/play_light.svg");
    QIcon continueIcon = QIcon(":/img/icons/media-skip-forward_light.svg");
    QIcon continueUntilIcon = QIcon(":/img/icons/continue_until_main.svg");
    QIcon stepIcon = QIcon(":/img/icons/step_light.svg");
    QIcon stepOverIcon = QIcon(":/img/icons/step_over_light.svg");

    QAction *actionStart = new QAction(startIcon, tr("Start debug"), parent);
    QAction *actionContinue = new QAction(continueIcon, tr("Continue"), parent);
    QAction *actionContinueUntilMain = new QAction(continueUntilIcon, tr("Continue until main"), parent);
    QAction *actionStep = new QAction(stepIcon, tr("Step"), parent);
    QAction *actionStepOver = new QAction(stepOverIcon, tr("Step over"), parent);
    addAction(actionStart);
    addAction(actionContinue);
    addAction(actionContinueUntilMain);
    addAction(actionStep);
    addAction(actionStepOver);

    connect(actionStep,              &QAction::triggered, Core(), &CutterCore::stepDebug);
    connect(actionStart,             &QAction::triggered, Core(), &CutterCore::startDebug);
    connect(actionStepOver,          &QAction::triggered, Core(), &CutterCore::stepOverDebug);
    connect(actionContinue,          &QAction::triggered, Core(), &CutterCore::continueDebug);
    connect(actionContinueUntilMain, &QAction::triggered, this,   &DebugToolbar::continueUntilMain);
}

void DebugToolbar::continueUntilMain()
{
    Core()->continueUntilDebug(tr("main"));
}