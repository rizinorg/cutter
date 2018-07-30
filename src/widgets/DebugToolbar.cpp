#include "DebugToolbar.h"
#include "MainWindow.h"
#include "dialogs/AttachProcDialog.h"

#include <QAction>
#include <QPainter>
#include <QToolButton>
#include <QMenu>
#include <QFileInfo>

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
    QIcon stepOutIcon = QIcon(":/img/icons/step_out_light.svg");

    actionStart = new QAction(startDebugIcon, tr("Start debug"), parent);
    actionStart->setShortcut(QKeySequence(Qt::Key_F9));
    actionStartEmul = new QAction(startEmulIcon, tr("Start emulation"), parent);
    actionAttach = new QAction(startAttachIcon, tr("Attach to process"), parent);
    actionStop = new QAction(stopIcon, tr("Stop debug"), parent);
    actionContinue = new QAction(continueIcon, tr("Continue"), parent);
    actionContinue->setShortcut(QKeySequence(Qt::Key_F5));
    actionContinueUntilMain = new QAction(continueUntilMainIcon, tr("Continue until main"), parent);
    actionContinueUntilCall = new QAction(continueUntilCallIcon, tr("Continue until call"), parent);
    actionContinueUntilSyscall = new QAction(continueUntilSyscallIcon, tr("Continue until syscall"), parent);
    actionStep = new QAction(stepIcon, tr("Step"), parent);
    actionStep->setShortcut(QKeySequence(Qt::Key_F7));
    actionStepOver = new QAction(stepOverIcon, tr("Step over"), parent);
    actionStepOver->setShortcut(QKeySequence(Qt::Key_F8));
    actionStepOut = new QAction(stepOutIcon, tr("Step out"), parent);
    actionStepOut->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_F8));

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
    addAction(actionStepOut);

    connect(actionStop, &QAction::triggered, Core(), &CutterCore::stopDebug);
    connect(actionStop, &QAction::triggered, [=]() {
        actionContinue->setVisible(true);
        actionStart->setVisible(true);
        actionStartEmul->setVisible(true);
        actionAttach->setVisible(true);
        actionContinueUntilMain->setVisible(true);
        actionStepOut->setVisible(true);
        this->colorToolbar(false);
        actionStop->setText("Stop debug");
        colorToolbar(false);
    });
    connect(actionStep, &QAction::triggered, Core(), &CutterCore::stepDebug);
    connect(actionStart, &QAction::triggered, [=]() {
        QString filename = Core()->getConfig("file.path").split(" ").first();
        QFileInfo info(filename);
        if (!Core()->currentlyDebugging && !info.isExecutable()) {
            QMessageBox msgBox;
            msgBox.setText(QString("File '%1' does not have executable permissions.").arg(filename));
            msgBox.exec();
            return;
        }
        colorToolbar(true);
        actionAttach->setVisible(false);
        actionStartEmul->setVisible(false);
        Core()->startDebug();
    });
    connect(actionAttach, &QAction::triggered, this, &DebugToolbar::attachProcessDialog);
    connect(actionStartEmul, &QAction::triggered, Core(), &CutterCore::startEmulation);
    connect(actionStartEmul, &QAction::triggered, [=]() {
        actionContinue->setVisible(false);
        actionStart->setVisible(false);
        actionAttach->setVisible(false);
        actionContinueUntilMain->setVisible(false);
        actionStepOut->setVisible(false);
        continueUntilButton->setDefaultAction(actionContinueUntilSyscall);
        actionStop->setText("Stop emulation");
        colorToolbar(true);
    });
    connect(actionStepOver, &QAction::triggered, Core(), &CutterCore::stepOverDebug);
    connect(actionStepOut, &QAction::triggered, Core(), &CutterCore::stepOutDebug);
    connect(actionContinue, &QAction::triggered, Core(), &CutterCore::continueDebug);
    connect(actionContinueUntilMain, &QAction::triggered, this, &DebugToolbar::continueUntilMain);
    connect(actionContinueUntilCall, &QAction::triggered, Core(), &CutterCore::continueUntilCall);
    connect(actionContinueUntilSyscall, &QAction::triggered, Core(), &CutterCore::continueUntilSyscall);
}

void DebugToolbar::continueUntilMain()
{
    Core()->continueUntilDebug(tr("main"));
}

void DebugToolbar::colorToolbar(bool p)
{
    if (p) {
        setStyleSheet("QToolBar {background: green;}");
    } else {
        setStyleSheet("");
    }
}

void DebugToolbar::attachProcessDialog()
{
    AttachProcDialog *dialog = new AttachProcDialog(this);
    bool success = false;
    while (!success) {
        success = true;
        if (dialog->exec()) {
            int pid = dialog->getPID();
            if (pid >= 0) {
                attachProcess(pid);
            } else {
                success = false;
                QMessageBox msgBox;
                msgBox.setText("Error attaching. No process selected!");
                msgBox.exec();
            }
        }
    }
    delete dialog;
}

void DebugToolbar::attachProcess(int pid)
{
    // hide unwanted buttons
    colorToolbar(true);
    actionStart->setVisible(false);
    actionStartEmul->setVisible(false);
    actionStop->setText("Detach from process");
    // attach
    Core()->attachDebug(pid);
}