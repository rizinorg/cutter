#include "DebugActions.h"
#include "core/MainWindow.h"
#include "dialogs/AttachProcDialog.h"
#include "dialogs/NativeDebugDialog.h"
#include "common/Configuration.h"
#include "common/Helpers.h"

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
    QIcon startEmulIcon = QIcon(":/img/icons/play_light_emul.svg");
    QIcon startAttachIcon = QIcon(":/img/icons/play_light_attach.svg");
    QIcon startRemoteIcon = QIcon(":/img/icons/play_light_remote.svg");
    stopIcon = QIcon(":/img/icons/media-stop_light.svg");
    restartIcon = QIcon(":/img/icons/spin_light.svg");
    detachIcon = QIcon(":/img/icons/detach_debugger.svg");
    startDebugIcon = QIcon(":/img/icons/play_light_debug.svg");
    continueIcon = QIcon(":/img/icons/media-skip-forward_light.svg");
    suspendIcon = QIcon(":/img/icons/media-suspend_light.svg");

    // define action labels
    QString startEmulLabel = tr("Start emulation");
    QString startAttachLabel = tr("Attach to process");
    QString startRemoteLabel = tr("Connect to a remote debugger");
    QString stopDebugLabel = tr("Stop debug");
    QString stopEmulLabel = tr("Stop emulation");
    QString restartEmulLabel = tr("Restart emulation");
    QString continueUMLabel = tr("Continue until main");
    QString continueUCLabel = tr("Continue until call");
    QString continueUSLabel = tr("Continue until syscall");
    QString stepLabel = tr("Step");
    QString stepOverLabel = tr("Step over");
    QString stepOutLabel = tr("Step out");
    suspendLabel = tr("Suspend the process");
    continueLabel = tr("Continue");
    restartDebugLabel = tr("Restart program");
    startDebugLabel = tr("Start debug");

    // define actions
    actionStart = new QAction(startDebugIcon, startDebugLabel, this);
    actionStart->setShortcut(QKeySequence(Qt::Key_F9));
    actionStartEmul = new QAction(startEmulIcon, startEmulLabel, this);
    actionAttach = new QAction(startAttachIcon, startAttachLabel, this);
    actionStartRemote = new QAction(startRemoteIcon, startRemoteLabel, this);
    actionStop = new QAction(stopIcon, stopDebugLabel, this);
    actionContinue = new QAction(continueIcon, continueLabel, this);
    actionContinue->setShortcut(QKeySequence(Qt::Key_F5));
    actionContinueUntilMain = new QAction(continueUMLabel, this);
    actionContinueUntilCall = new QAction(continueUCLabel, this);
    actionContinueUntilSyscall = new QAction(continueUSLabel, this);
    actionStep = new QAction(stepLabel, this);
    actionStep->setShortcut(QKeySequence(Qt::Key_F7));
    actionStepOver = new QAction(stepOverLabel, this);
    actionStepOver->setShortcut(QKeySequence(Qt::Key_F8));
    actionStepOut = new QAction(stepOutLabel, this);
    actionStepOut->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_F8));

    QToolButton *startButton = new QToolButton;
    startButton->setPopupMode(QToolButton::MenuButtonPopup);
    connect(startButton, &QToolButton::triggered, startButton, &QToolButton::setDefaultAction);
    QMenu *startMenu = new QMenu(startButton);

    // only emulation is currently allowed
    startMenu->addAction(actionStart);
    startMenu->addAction(actionStartEmul);
    startMenu->addAction(actionAttach);
    startMenu->addAction(actionStartRemote);
    startButton->setDefaultAction(actionStart);
    startButton->setMenu(startMenu);

    continueUntilButton = new QToolButton;
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
    toolBar->addAction(actionStepOver);
    toolBar->addAction(actionStep);
    toolBar->addAction(actionStepOut);

    allActions = {actionStop, actionAllContinues, actionContinue, actionContinueUntilCall, actionContinueUntilMain, actionContinueUntilSyscall, actionStep, actionStepOut, actionStepOver};
    // hide allactions
    setAllActionsVisible(false);

    // Toggle all buttons except restart, suspend(=continue) and stop since those are
    // necessary to avoid staying stuck
    toggleActions = {actionStepOver, actionStep, actionStepOut, actionContinueUntilMain,
        actionContinueUntilCall, actionContinueUntilSyscall};
    toggleConnectionActions = {actionAttach, actionStartRemote};

    connect(Core(), &CutterCore::debugProcessFinished, this, [ = ](int pid) {
        QMessageBox msgBox;
        msgBox.setText(tr("Debugged process exited (") + QString::number(pid) + ")");
        msgBox.exec();
    });

    connect(Core(), &CutterCore::debugTaskStateChanged, this, [ = ]() {
        bool disableToolbar = Core()->isDebugTaskInProgress();
        if (Core()->currentlyDebugging) {
            for (QAction *a : toggleActions) {
                a->setDisabled(disableToolbar);
            }
            // Suspend should only be available when other icons are disabled
            if (disableToolbar) {
                actionContinue->setText(suspendLabel);
                actionContinue->setIcon(suspendIcon);
            } else {
                actionContinue->setText(continueLabel);
                actionContinue->setIcon(continueIcon);
            }
        } else {
            for (QAction *a : toggleConnectionActions) {
                a->setDisabled(disableToolbar);
            }
        }
    });

    connect(actionStop, &QAction::triggered, Core(), &CutterCore::stopDebug);
    connect(actionStop, &QAction::triggered, [ = ]() {
        actionStart->setVisible(true);
        actionStartEmul->setVisible(true);
        actionAttach->setVisible(true);
        actionStartRemote->setVisible(true);
        actionStop->setText(stopDebugLabel);
        actionStop->setIcon(stopIcon);
        actionStart->setText(startDebugLabel);
        actionStart->setIcon(startDebugIcon);
        actionStartEmul->setText(startEmulLabel);
        actionStartEmul->setIcon(startEmulIcon);
        continueUntilButton->setDefaultAction(actionContinueUntilMain);
        setAllActionsVisible(false);
    });
    connect(actionStep, &QAction::triggered, Core(), &CutterCore::stepDebug);
    connect(actionStart, &QAction::triggered, this, &DebugActions::startDebug);

    connect(actionAttach, &QAction::triggered, this, &DebugActions::attachProcessDialog);
    connect(actionStartRemote, &QAction::triggered, this, &DebugActions::attachRemoteDialog);
    connect(Core(), &CutterCore::attachedRemote, this, &DebugActions::onAttachedRemoteDebugger);
    connect(actionStartEmul, &QAction::triggered, Core(), &CutterCore::startEmulation);
    connect(actionStartEmul, &QAction::triggered, [ = ]() {
        setAllActionsVisible(true);
        actionStart->setVisible(false);
        actionAttach->setVisible(false);
        actionStartRemote->setVisible(false);
        actionContinueUntilMain->setVisible(false);
        actionStepOut->setVisible(false);
        continueUntilButton->setDefaultAction(actionContinueUntilSyscall);
        actionStartEmul->setText(restartEmulLabel);
        actionStartEmul->setIcon(restartIcon);
        actionStop->setText(stopEmulLabel);
    });
    connect(actionStepOver, &QAction::triggered, Core(), &CutterCore::stepOverDebug);
    connect(actionStepOut, &QAction::triggered, Core(), &CutterCore::stepOutDebug);
    connect(actionContinueUntilMain, &QAction::triggered, this, &DebugActions::continueUntilMain);
    connect(actionContinueUntilCall, &QAction::triggered, Core(), &CutterCore::continueUntilCall);
    connect(actionContinueUntilSyscall, &QAction::triggered, Core(), &CutterCore::continueUntilSyscall);
    connect(actionContinue, &QAction::triggered, Core(), [=]() {
        // Switch between continue and suspend depending on the debugger's state
        if (Core()->isDebugTaskInProgress()) {
            Core()->suspendDebug();
        } else {
            Core()->continueDebug();
        }
    });

    connect(Config(), &Configuration::interfaceThemeChanged, this, &DebugActions::chooseThemeIcons);
    chooseThemeIcons();
}

void DebugActions::setButtonVisibleIfMainExists()
{
    int mainExists = Core()->cmd("f?sym.main; ??").toInt();
    // if main is not a flag we hide the continue until main button
    if (!mainExists) {
        actionContinueUntilMain->setVisible(false);
        continueUntilButton->setDefaultAction(actionContinueUntilCall);
    }
}

void DebugActions::showDebugWarning()
{
    if (!acceptedDebugWarning) {
        acceptedDebugWarning = true;
        QMessageBox msgBox;
        msgBox.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
        msgBox.setText(tr("Debug is currently in beta.\n") +
            tr("If you encounter any problems or have suggestions, please submit an issue to https://github.com/radareorg/cutter/issues"));
        msgBox.exec();
    }
}

void DebugActions::continueUntilMain()
{
    QString mainAddr = Core()->cmd("?v sym.main");
    Core()->continueUntilDebug(mainAddr);
}

void DebugActions::attachRemoteDebugger()
{
    QString stopAttachLabel = tr("Detach from process");
    // Hide unwanted buttons
    setAllActionsVisible(true);
    actionStart->setVisible(false);
    actionStartRemote->setVisible(false);
    actionStartEmul->setVisible(false);
    actionStop->setText(stopAttachLabel);
}

void DebugActions::onAttachedRemoteDebugger(bool successfully)
{
    if (!successfully) {
        QMessageBox msgBox;
        msgBox.setText(tr("Error connecting."));
        msgBox.exec();
        attachRemoteDialog();
    } else {
        delete remoteDialog;
        remoteDialog = nullptr;
        attachRemoteDebugger();
    }
}

void DebugActions::attachRemoteDialog()
{
    showDebugWarning();

    if (!remoteDialog) {
        remoteDialog = new RemoteDebugDialog(main);
    }
    QMessageBox msgBox;
    bool success = false;
    while (!success) {
        success = true;
        if (remoteDialog->exec()) {
            if (!remoteDialog->validate()) {
                success = false;
                continue;
            }

            Core()->attachRemote(remoteDialog->getUri());
        }
    }
}

void DebugActions::attachProcessDialog()
{
    showDebugWarning();

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
    actionStartRemote->setVisible(false);
    actionStartEmul->setVisible(false);
    actionStop->setText(stopAttachLabel);
    actionStop->setIcon(detachIcon);
    // attach
    Core()->attachDebug(pid);
}

void DebugActions::startDebug()
{
    // check if file is executable before starting debug
    QString filename = Core()->getConfig("file.path");

    QFileInfo info(filename);
    if (!Core()->currentlyDebugging && !info.isExecutable()) {
        QMessageBox msgBox;
        msgBox.setText(tr("File '%1' does not have executable permissions.").arg(filename));
        msgBox.exec();
        return;
    }

    showDebugWarning();

    NativeDebugDialog dialog(main);
    dialog.setArgs(Core()->getConfig("dbg.args"));
    QString args;
    if (dialog.exec()) {
        args = dialog.getArgs();
    } else {
        return;
    }

    // Update dbg.args with the new args
    Core()->setConfig("dbg.args", args);

    setAllActionsVisible(true);
    actionAttach->setVisible(false);
    actionStartRemote->setVisible(false);
    actionStartEmul->setVisible(false);
    actionStart->setText(restartDebugLabel);
    actionStart->setIcon(restartIcon);
    setButtonVisibleIfMainExists();

    Core()->startDebug();
}

void DebugActions::setAllActionsVisible(bool visible)
{
    for (QAction *action : allActions) {
        action->setVisible(visible);
    }
}

/**
 * @brief When theme changed, change icons which have a special version for the theme.
 */
void DebugActions::chooseThemeIcons()
{
    // List of QActions which have alternative icons in different themes
    const QList<QPair<void*, QString>> kSupportedIconsNames {
        { actionStep, QStringLiteral("step_into.svg") },
        { actionStepOver, QStringLiteral("step_over.svg") },
        { actionStepOut, QStringLiteral("step_out.svg") },
        { actionContinueUntilMain, QStringLiteral("continue_until_main.svg") },
        { actionContinueUntilCall, QStringLiteral("continue_until_call.svg") },
        { actionContinueUntilSyscall, QStringLiteral("continue_until_syscall.svg") },
    };


    // Set the correct icon for the QAction
    qhelpers::setThemeIcons(kSupportedIconsNames, [](void *obj, const QIcon &icon) {
        static_cast<QAction*>(obj)->setIcon(icon);
    });
}
