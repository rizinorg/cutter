#include "DebugActions.h"

#include "common/Configuration.h"
#include "common/Helpers.h"
#include "core/MainWindow.h"
#include "dialogs/AttachProcDialog.h"
#include "dialogs/NativeDebugDialog.h"
#include "shortcuts/ShortcutManager.h"

#include <QFileInfo>
#include <QList>
#include <QMenu>
#include <QPainter>
#include <QSettings>
#include <QToolBar>
#include <QToolButton>

DebugActions::DebugActions(QToolBar *toolBar, MainWindow *main)
    : QObject(main), continueUntilButton(new QToolButton), main(main)
{
    setObjectName("DebugActions");
    // setIconSize(QSize(16, 16));

    // define icons
    const QIcon startEmulIcon = QIcon(":/img/icons/play_light_emul.svg");
    const QIcon startAttachIcon = QIcon(":/img/icons/play_light_attach.svg");
    const QIcon startRemoteIcon = QIcon(":/img/icons/play_light_remote.svg");
    const QIcon continueBackIcon = QIcon(":/img/icons/reverse_continue.svg");
    const QIcon stepBackIcon = QIcon(":/img/icons/reverse_step.svg");
    startTraceIcon = QIcon(":/img/icons/start_trace.svg");
    stopTraceIcon = QIcon(":/img/icons/stop_trace.svg");
    stopIcon = QIcon(":/img/icons/media-stop_light.svg");
    restartIcon = QIcon(":/img/icons/spin_light.svg");
    detachIcon = QIcon(":/img/icons/detach_debugger.svg");
    startDebugIcon = QIcon(":/img/icons/play_light_debug.svg");
    continueIcon = QIcon(":/img/icons/media-skip-forward_light.svg");
    suspendIcon = QIcon(":/img/icons/media-suspend_light.svg");

    // define action labels
    const QString startEmulLabel = tr("Start emulation");
    const QString startAttachLabel = tr("Attach to process");
    const QString startRemoteLabel = tr("Connect to a remote debugger");
    const QString stopDebugLabel = tr("Stop debug");
    const QString stopEmulLabel = tr("Stop emulation");
    const QString restartEmulLabel = tr("Restart emulation");
    const QString continueUMLabel = tr("Continue until main");
    const QString continueUCLabel = tr("Continue until call");
    const QString continueUSLabel = tr("Continue until syscall");
    startTraceLabel = tr("Start trace session");
    stopTraceLabel = tr("Stop trace session");
    suspendLabel = tr("Suspend the process");
    continueLabel = tr("Continue");
    restartDebugLabel = tr("Restart program");
    startDebugLabel = tr("Start debug");

    // define actions
    actionStart = Shortcuts()->makeAction("Debug.start", this);
    actionContinue = Shortcuts()->makeAction("Debug.continue", this);
    actionContinueBack = Shortcuts()->makeAction("Debug.continueBack", this);
    actionStep = Shortcuts()->makeAction("Debug.step", this);
    actionStepOver = Shortcuts()->makeAction("Debug.stepOver", this);
    actionStepOut = Shortcuts()->makeAction("Debug.stepOut", this);
    actionStepBack = Shortcuts()->makeAction("Debug.stepBack", this);
    actionStart->setIcon(startDebugIcon);
    actionContinue->setIcon(continueIcon);
    actionContinueBack->setIcon(continueBackIcon);
    actionStepBack->setIcon(stepBackIcon);

    actionStartEmul = new QAction(startEmulIcon, startEmulLabel, this);
    actionAttach = new QAction(startAttachIcon, startAttachLabel, this);
    actionStartRemote = new QAction(startRemoteIcon, startRemoteLabel, this);
    actionStop = new QAction(stopIcon, stopDebugLabel, this);
    actionContinueUntilMain = new QAction(continueUMLabel, this);
    actionContinueUntilCall = new QAction(continueUCLabel, this);
    actionContinueUntilSyscall = new QAction(continueUSLabel, this);
    actionTrace = new QAction(startTraceIcon, startTraceLabel, this);

    auto *startButton = new QToolButton;
    startButton->setPopupMode(QToolButton::MenuButtonPopup);
    connect(startButton, &QToolButton::triggered, startButton, &QToolButton::setDefaultAction);
    auto *startMenu = new QMenu(startButton);

    startMenu->addAction(actionStart);
    startMenu->addAction(actionStartEmul);
    startMenu->addAction(actionAttach);
    startMenu->addAction(actionStartRemote);
    startButton->setDefaultAction(actionStart);
    startButton->setMenu(startMenu);

    continueUntilButton->setPopupMode(QToolButton::MenuButtonPopup);
    connect(continueUntilButton, &QToolButton::triggered, continueUntilButton,
            &QToolButton::setDefaultAction);
    auto *continueUntilMenu = new QMenu(continueUntilButton);
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
    toolBar->addAction(actionStepBack);
    toolBar->addAction(actionContinueBack);
    toolBar->addAction(actionTrace);

    allActions = { actionStop,
                   actionAllContinues,
                   actionContinue,
                   actionContinueUntilCall,
                   actionContinueUntilMain,
                   actionContinueUntilSyscall,
                   actionStep,
                   actionStepOut,
                   actionStepOver,
                   actionContinueBack,
                   actionStepBack,
                   actionTrace };

    // Hide all actions
    setAllActionsVisible(false);

    // Toggle all buttons except reverse step/continue which are handled separately and
    // restart, suspend(=continue) and stop since those are necessary to avoid freezing
    toggleActions = { actionStepOver,
                      actionStep,
                      actionStepOut,
                      actionContinueUntilMain,
                      actionContinueUntilCall,
                      actionContinueUntilSyscall,
                      actionTrace };
    toggleConnectionActions = { actionAttach, actionStartRemote };
    reverseActions = { actionStepBack, actionContinueBack };

    connect(Core(), &CutterCore::debugProcessFinished, this, [=](int pid) {
        QMessageBox msgBox(main);
        msgBox.setText(tr("Debugged process exited (") + QString::number(pid) + ")");
        msgBox.exec();
    });

    connect(Core(), &CutterCore::debugTaskStateChanged, this, [=, this]() {
        const bool disableToolbar = Core()->isDebugTaskInProgress();
        if (Core()->currentlyDebugging) {
            for (auto a : std::as_const(toggleActions)) {
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
            for (auto a : std::as_const(reverseActions)) {
                a->setVisible(Core()->currentlyTracing);
                a->setDisabled(disableToolbar);
            }
        } else {
            for (auto a : std::as_const(toggleConnectionActions)) {
                a->setDisabled(disableToolbar);
            }
        }
    });

    connect(actionStop, &QAction::triggered, Core(), &CutterCore::stopDebug);
    connect(actionStop, &QAction::triggered, [=, this]() {
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
    connect(actionStepBack, &QAction::triggered, Core(), &CutterCore::stepBackDebug);

    connect(actionStart, &QAction::triggered, this, &DebugActions::startDebug);

    connect(actionAttach, &QAction::triggered, this, &DebugActions::attachProcessDialog);
    connect(actionStartRemote, &QAction::triggered, this, &DebugActions::attachRemoteDialog);
    connect(Core(), &CutterCore::attachedRemote, this, &DebugActions::onAttachedRemoteDebugger);
    connect(actionStartEmul, &QAction::triggered, Core(), &CutterCore::startEmulation);
    connect(actionStartEmul, &QAction::triggered, [=, this]() {
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
        // Reverse debug actions aren't visible until we start tracing
        for (auto a : std::as_const(reverseActions)) {
            a->setVisible(false);
        }
    });
    connect(actionStepOver, &QAction::triggered, Core(), &CutterCore::stepOverDebug);
    connect(actionStepOut, &QAction::triggered, Core(), &CutterCore::stepOutDebug);
    connect(actionContinueUntilMain, &QAction::triggered, this, &DebugActions::continueUntilMain);
    connect(actionContinueUntilCall, &QAction::triggered, Core(), &CutterCore::continueUntilCall);
    connect(actionContinueUntilSyscall, &QAction::triggered, Core(),
            &CutterCore::continueUntilSyscall);
    connect(actionContinueBack, &QAction::triggered, Core(), &CutterCore::continueBackDebug);
    connect(actionContinue, &QAction::triggered, Core(), [=]() {
        // Switch between continue and suspend depending on the debugger's state
        if (Core()->isDebugTaskInProgress()) {
            Core()->suspendDebug();
        } else {
            Core()->continueDebug();
        }
    });

    connect(actionTrace, &QAction::triggered, Core(), [=, this]() {
        // Check if a debug session was created to switch between start and stop
        if (!Core()->currentlyTracing) {
            Core()->startTraceSession();
            actionTrace->setText(stopTraceLabel);
            actionTrace->setIcon(stopTraceIcon);
        } else {
            Core()->stopTraceSession();
            actionTrace->setText(startTraceLabel);
            actionTrace->setIcon(startTraceIcon);
        }
    });

    connect(Config(), &Configuration::interfaceThemeChanged, this, &DebugActions::chooseThemeIcons);
    chooseThemeIcons();
}

void DebugActions::setButtonVisibleIfMainExists()
{
    RzCoreLocked core = Core()->lock();
    // if main is not a flag we hide the continue until main button
    if (!rz_flag_get(core->flags, "sym.main") && !rz_flag_get(core->flags, "main")) {
        actionContinueUntilMain->setVisible(false);
        continueUntilButton->setDefaultAction(actionContinueUntilCall);
    }
}

void DebugActions::showDebugWarning()
{
    if (!acceptedDebugWarning) {
        acceptedDebugWarning = true;
        QMessageBox msgBox(main);
        msgBox.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
        msgBox.setText(tr("Debug is currently in beta.\n")
                       + tr("If you encounter any problems or have suggestions, please submit an "
                            "issue to https://github.com/rizinorg/cutter/issues"));
        msgBox.exec();
    }
}

void DebugActions::continueUntilMain()
{
    auto core = Core()->lock();
    const RzFlagItem *mainFlag = rz_flag_get(core->flags, "sym.main");
    if (!mainFlag) {
        mainFlag = rz_flag_get(core->flags, "main");
        if (!mainFlag) {
            return;
        }
    }
    Core()->continueUntilDebug(mainFlag->offset);
}

void DebugActions::attachRemoteDebugger()
{
    const QString stopAttachLabel = tr("Detach from process");
    // Hide unwanted buttons
    setAllActionsVisible(true);
    actionStart->setVisible(false);
    actionStartRemote->setVisible(false);
    actionStartEmul->setVisible(false);
    actionStop->setText(stopAttachLabel);
}

void DebugActions::onAttachedRemoteDebugger(bool successfully)
{
    // TODO(#2829): Investigate why this is happening
    if (remoteDialog == nullptr) {
        return;
    }

    if (!successfully) {
        QMessageBox msgBox(main);
        msgBox.setText(tr("Error connecting."));
        msgBox.exec();
        attachRemoteDialog();
    } else {
        QSettings settings;
        QStringList ips = settings.value("recentIpList").toStringList();
        ips.removeAll(remoteDialog->getUri());
        ips.prepend(remoteDialog->getUri());
        settings.setValue("recentIpList", ips);

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
    const QMessageBox msgBox(main);
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
            const int pid = dialog.getPID();
            if (pid >= 0) {
                attachProcess(pid);
            } else {
                success = false;
                QMessageBox msgBox(main);
                msgBox.setText(tr("Error attaching. No process selected!"));
                msgBox.exec();
            }
        }
    }
}

void DebugActions::attachProcess(int pid)
{
    const QString stopAttachLabel = tr("Detach from process");
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
    const QString filename = Core()->getConfig("file.path");

    const QFileInfo info(filename);
    if (!Core()->currentlyDebugging && !info.isExecutable()) {
        QMessageBox msgBox(main);
        msgBox.setText(tr("File '%1' does not have executable permissions.").arg(filename));
        msgBox.exec();
        return;
    }

    showDebugWarning();

    NativeDebugDialog dialog(main);
    dialog.setArgs(Core()->getConfig("dbg.args"));
    dialog.setProfilePath(Core()->getConfig("dbg.profile"));
    if (!dialog.exec()) {
        return;
    }

    switch (dialog.getSelectedMethod()) {
    case DebugConfigMethod::CommandLine: {
        Core()->setConfig("dbg.args", dialog.getArgs());
        Core()->setConfig("dbg.profile", ""); // profile overrides args, so remove it
        break;
    }
    case DebugConfigMethod::RzRunProfile: {
        Core()->setConfig("dbg.profile", dialog.getProfilePath());
        break;
    }
    case DebugConfigMethod::RzRunDirectives: {
        Core()->setProfileDirectives(dialog.getDirectives());
        break;
    }
    default:
        break;
    }

    setAllActionsVisible(true);
    actionAttach->setVisible(false);
    actionStartRemote->setVisible(false);
    actionStartEmul->setVisible(false);
    actionStart->setText(restartDebugLabel);
    actionStart->setIcon(restartIcon);
    setButtonVisibleIfMainExists();

    // Reverse debug actions aren't visible until we start tracing
    for (auto a : std::as_const(reverseActions)) {
        a->setVisible(false);
    }
    actionTrace->setText(startTraceLabel);
    actionTrace->setIcon(startTraceIcon);

    Core()->startDebug();
}

void DebugActions::setAllActionsVisible(bool visible)
{
    for (auto action : std::as_const(allActions)) {
        action->setVisible(visible);
    }
}

void DebugActions::chooseThemeIcons()
{
    // List of QActions which have alternative icons in different themes
    const QList<QPair<void *, QString>> kSupportedIconsNames {
        { actionStep, QStringLiteral("step_into.svg") },
        { actionStepOver, QStringLiteral("step_over.svg") },
        { actionStepOut, QStringLiteral("step_out.svg") },
        { actionContinueUntilMain, QStringLiteral("continue_until_main.svg") },
        { actionContinueUntilCall, QStringLiteral("continue_until_call.svg") },
        { actionContinueUntilSyscall, QStringLiteral("continue_until_syscall.svg") },
    };

    // Set the correct icon for the QAction
    qhelpers::setThemeIcons(kSupportedIconsNames, [](void *obj, const QIcon &icon) {
        static_cast<QAction *>(obj)->setIcon(icon);
    });
}
