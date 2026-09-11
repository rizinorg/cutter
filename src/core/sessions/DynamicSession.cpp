#include "DynamicSession.h"

#include "RizinTask.h"
#include "RizinWrapper.h"
#include "dialogs/RizinTaskDialog.h"

static DynamicSession *session = nullptr;

DynamicSession *DynamicSession::instance()
{
    if (!session) {
        session = new DynamicSession();
    }
    return session;
}

RizinWrapper *DynamicSession::getWrapper() const
{
    return wrapper;
}

DynamicSession::DynamicSession(QObject *parent) : CoreSession(parent)
{
    wrapper->setDebugStateProvider([this]() { return this->currentlyDebugging; },
                                   [this]() { return this->currentlyEmulating; });
}

void DynamicSession::setCurrentDebugThread(int tid)
{
    if (!asyncTask(
                [=](RzCore *core) {
                    rz_debug_select(core->dbg, core->dbg->pid, tid);
                    return (void *)nullptr;
                },
                debugTask)) {
        return;
    }

    emit debugTaskStateChanged();
    connect(debugTask.get(), &RizinTask::finished, this, [this]() {
        debugTask.reset();
        emit wrapper->registersChanged();
        emit refreshCodeViews();
        emit wrapper->stackChanged();
        syncAndSeekProgramCounter();
        emit switchedThread();
        emit debugTaskStateChanged();
    });

    debugTask->startTask();
}

void DynamicSession::setCurrentDebugProcess(int pid)
{
    if (!currentlyDebugging
        || !asyncTask(
                [=](RzCore *core) {
                    rz_debug_select(core->dbg, pid, core->dbg->tid);
                    core->dbg->main_pid = pid;
                    return (void *)nullptr;
                },
                debugTask)) {
        return;
    }

    emit debugTaskStateChanged();
    connect(debugTask.get(), &RizinTask::finished, this, [this]() {
        debugTask.reset();
        emit wrapper->registersChanged();
        emit refreshCodeViews();
        emit wrapper->stackChanged();
        emit wrapper->flagsChanged();
        syncAndSeekProgramCounter();
        emit switchedProcess();
        emit debugTaskStateChanged();
    });

    debugTask->startTask();
}

void DynamicSession::startDebug()
{
    {
        auto rizin = lock();
        if (!currentlyDebugging) {
            offsetPriorDebugging = rizin->getOffset();
        }
        currentlyOpenFile = rizin->getConfig("file.path");
    }

    if (!asyncTask(
                [](RzCore *core) {
                    rz_core_file_reopen_debug(core, "");
                    return nullptr;
                },
                debugTask)) {
        return;
    }

    emit debugTaskStateChanged();

    connect(debugTask.get(), &RizinTask::finished, this, [this]() {
        delete debugTaskDialog;
        debugTask.reset();

        emit wrapper->registersChanged();
        if (!currentlyDebugging) {
            LOCK(this, { rizin->setConfig("asm.flags", false); });
            currentlyDebugging = true;
            emit toggleDebugView();
            emit refreshCodeViews();
        }

        emit wrapper->stackChanged();
        emit codeRebased();
        emit debugTaskStateChanged();
    });

    debugTaskDialog = new RizinTaskDialog(debugTask);
    debugTaskDialog->setBreakOnClose(true);
    debugTaskDialog->setAttribute(Qt::WA_DeleteOnClose);
    debugTaskDialog->setDesc(tr("Starting native debug..."));
    debugTaskDialog->show();

    debugTask->startTask();
}

void DynamicSession::startEmulation()
{
    if (!currentlyDebugging) {
        auto rizin = lock();
        offsetPriorDebugging = rizin->getOffset();
    }

    // clear registers, init esil state, stack, progcounter at current seek
    asyncTask(
            [&](RzCore *core) {
                rz_core_analysis_esil_reinit(core);
                rz_core_analysis_esil_init_mem(core, nullptr, UT64_MAX, UT32_MAX);
                rz_core_analysis_esil_init_regs(core);
                return nullptr;
            },
            debugTask);

    emit debugTaskStateChanged();

    connect(debugTask.get(), &RizinTask::finished, this, [this]() {
        delete debugTaskDialog;
        debugTask.reset();

        if (!currentlyDebugging || !currentlyEmulating) {
            {
                auto rizin = lock();
                // prevent register flags from appearing during debug/emul
                rizin->setConfig("asm.flags", false);
                // allows to view self-modifying code changes or other binary changes
                rizin->setConfig("io.cache", true);
            }

            currentlyDebugging = true;
            currentlyEmulating = true;
            emit toggleDebugView();
        }

        emit wrapper->registersChanged();
        emit wrapper->stackChanged();
        emit codeRebased();
        emit refreshCodeViews();
        emit debugTaskStateChanged();
    });

    debugTaskDialog = new RizinTaskDialog(debugTask);
    debugTaskDialog->setBreakOnClose(true);
    debugTaskDialog->setAttribute(Qt::WA_DeleteOnClose);
    debugTaskDialog->setDesc(tr("Starting emulation..."));
    debugTaskDialog->show();

    debugTask->startTask();
}

void DynamicSession::attachRemote(const QString &uri)
{
    if (!currentlyDebugging) {
        auto rizin = lock();
        offsetPriorDebugging = rizin->getOffset();
    }

    // connect to a debugger with the given plugin
    if (!asyncTask(
                [&](RzCore *core) {
                    rz_config_set_b(core->config, "cfg.debug", true);
                    rz_core_file_reopen_remote_debug(core, uri.toStdString().c_str(), 0);
                    return nullptr;
                },
                debugTask)) {
        return;
    }
    emit debugTaskStateChanged();

    connect(debugTask.get(), &RizinTask::finished, this, [this, uri]() {
        delete debugTaskDialog;
        debugTask.reset();
        RVA pcValue;
        // Check if we actually connected
        bool connected = false;
        {
            auto rizin = lock();
            const RzList *descs = rz_id_storage_list(rizin.core()->io->files);
            RzListIter *it;
            RzIODesc *desc;
            CutterRzListForeach (descs, it, RzIODesc, desc) {
                const QString fileUri = QString(desc->uri);
                if (!fileUri.compare(uri)) {
                    connected = true;
                }
            }
            pcValue = rizin->getProgramCounterValue();
        }
        seekAndShow(pcValue);

        if (!connected) {
            emit attachedRemote(false);
            emit debugTaskStateChanged();
            return;
        }

        emit wrapper->registersChanged();
        if (!currentlyDebugging || !currentlyEmulating) {
            // prevent register flags from appearing during debug/emul
            LOCK(this, { rizin->setConfig("asm.flags", false); });
            currentlyDebugging = true;
            emit toggleDebugView();
        }

        currentlyRemoteDebugging = true;
        emit codeRebased();
        emit attachedRemote(true);
        emit debugTaskStateChanged();
    });

    debugTaskDialog = new RizinTaskDialog(debugTask);
    debugTaskDialog->setBreakOnClose(true);
    debugTaskDialog->setAttribute(Qt::WA_DeleteOnClose);
    debugTaskDialog->setDesc(tr("Connecting to: ") + uri);
    debugTaskDialog->show();

    debugTask->startTask();
}

void DynamicSession::attachDebug(int pid)
{
    if (!currentlyDebugging) {
        auto rizin = lock();
        offsetPriorDebugging = rizin->getOffset();
    }

    if (!asyncTask(
                [&](RzCore *core) {
                    // cannot use setConfig because core is
                    // already locked, which causes a deadlock
                    rz_config_set_b(core->config, "cfg.debug", true);
                    auto uri = rz_str_newf("dbg://%d", pid);
                    if (currentlyOpenFile.isEmpty()) {
                        rz_core_file_open_load(core, uri, 0, RZ_PERM_R, false);
                    } else {
                        rz_core_file_reopen_remote_debug(core, uri, 0);
                    }
                    free(uri);
                    return nullptr;
                },
                debugTask)) {
        return;
    }
    emit debugTaskStateChanged();

    connect(debugTask.get(), &RizinTask::finished, this, [this, pid]() {
        delete debugTaskDialog;
        debugTask.reset();

        syncAndSeekProgramCounter();
        if (!currentlyDebugging || !currentlyEmulating) {
            {
                auto rizin = lock();
                // prevent register flags from appearing during debug/emul
                rizin->setConfig("asm.flags", false);
                currentlyOpenFile = rizin->getConfig("file.path");
            }
            currentlyDebugging = true;
            currentlyAttachedToPID = pid;
            emit toggleDebugView();
        }

        emit codeRebased();
        emit debugTaskStateChanged();
    });

    debugTaskDialog = new RizinTaskDialog(debugTask);
    debugTaskDialog->setBreakOnClose(true);
    debugTaskDialog->setAttribute(Qt::WA_DeleteOnClose);
    debugTaskDialog->setDesc(tr("Attaching to process (") + QString::number(pid) + ")...");
    debugTaskDialog->show();

    debugTask->startTask();
}

void DynamicSession::suspendDebug()
{
    debugTask->breakTask();
    debugTask->joinTask();
}

void DynamicSession::stopDebug()
{
    if (!currentlyDebugging) {
        return;
    }

    if (debugTask) {
        suspendDebug();
    }

    currentlyDebugging = false;
    currentlyTracing = false;
    currentlyRemoteDebugging = false;
    emit debugTaskStateChanged();

    {
        auto rizin = lock();
        auto core = rizin.core();
        if (currentlyEmulating) {
            rz_core_analysis_esil_init_mem_del(core, nullptr, UT64_MAX, UT32_MAX);
            rz_core_analysis_esil_deinit(core);
            rizin->resetWriteCache();
            rz_core_debug_clear_register_flags(core);
            rz_core_analysis_esil_trace_stop(core);
            currentlyEmulating = false;
        } else {
            // ensure we have opened a file.
            if (core->io->desc) {
                rz_core_debug_process_close(core);
            }
            currentlyAttachedToPID = -1;
        }
        rizin->setConfig("asm.flags", true);
        rizin->setConfig("io.cache", false);
    }

    syncAndSeekProgramCounter();
    emit codeRebased();
    emit toggleDebugView();
    offsetPriorDebugging = LOCK(this, { return rizin->getOffset(); });
    emit debugTaskStateChanged();
}

void DynamicSession::syncAndSeekProgramCounter()
{
    const RVA offset = LOCK(this, { return rizin->getProgramCounterValue(); });
    seekAndShow(offset);
    emit wrapper->registersChanged();
}

void DynamicSession::continueDebug()
{
    if (!currentlyDebugging) {
        return;
    }

    if (currentlyEmulating) {
        if (!asyncTask(
                    [](RzCore *core) {
                        rz_core_esil_step(core, UT64_MAX, "0", nullptr, false);
                        rz_core_reg_update_flags(core);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    } else {
        if (!asyncTask(
                    [](RzCore *core) {
                        rz_debug_continue(core->dbg);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    }

    emit debugTaskStateChanged();
    connect(debugTask.get(), &RizinTask::finished, this, [this]() {
        debugTask.reset();
        syncAndSeekProgramCounter();
        emit refreshCodeViews();
        emit debugTaskStateChanged();
    });

    debugTask->startTask();
}

void DynamicSession::continueBackDebug()
{
    if (!currentlyDebugging) {
        return;
    }

    if (currentlyEmulating) {
        if (!asyncTask(
                    [](RzCore *core) {
                        rz_core_esil_continue_back(core);
                        rz_core_reg_update_flags(core);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    } else {
        if (!asyncTask(
                    [](RzCore *core) {
                        rz_debug_continue_back(core->dbg);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    }
    emit debugTaskStateChanged();

    connect(debugTask.get(), &RizinTask::finished, this, [this]() {
        debugTask.reset();
        syncAndSeekProgramCounter();
        emit refreshCodeViews();
        emit debugTaskStateChanged();
    });

    debugTask->startTask();
}

void DynamicSession::continueUntilDebug(ut64 offset)
{
    if (!currentlyDebugging) {
        return;
    }

    if (currentlyEmulating) {
        if (!asyncTask(
                    [=](RzCore *core) {
                        rz_core_esil_step(core, offset, nullptr, nullptr, false);
                        rz_core_reg_update_flags(core);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    } else {
        if (!asyncTask(
                    [=](RzCore *core) {
                        rz_core_debug_continue_until(core, offset);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    }
    emit debugTaskStateChanged();
    connect(debugTask.get(), &RizinTask::finished, this, [this]() {
        debugTask.reset();
        syncAndSeekProgramCounter();
        emit refreshCodeViews();
        emit debugTaskStateChanged();
    });
    debugTask->startTask();
}

void DynamicSession::continueUntilCall()
{
    if (!currentlyDebugging) {
        return;
    }

    if (currentlyEmulating) {
        if (!asyncTask(
                    [](RzCore *core) {
                        rz_core_analysis_continue_until_call(core);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    } else {
        if (!asyncTask(
                    [](RzCore *core) {
                        rz_core_debug_step_one(core, 0);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    }

    emit debugTaskStateChanged();
    connect(debugTask.get(), &RizinTask::finished, this, [this]() {
        debugTask.reset();
        syncAndSeekProgramCounter();
        emit refreshCodeViews();
        emit debugTaskStateChanged();
    });

    debugTask->startTask();
}

void DynamicSession::continueUntilSyscall()
{
    if (!currentlyDebugging) {
        return;
    }

    if (currentlyEmulating) {
        if (!asyncTask(
                    [](RzCore *core) {
                        rz_core_analysis_continue_until_syscall(core);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    } else {
        if (!asyncTask(
                    [](RzCore *core) {
                        rz_cons_break_push(
                                [](void *x) { rz_debug_stop(reinterpret_cast<RzDebug *>(x)); },
                                core->dbg);
                        rz_reg_arena_swap(core->dbg->reg, true);
                        rz_debug_continue_syscalls(core->dbg, nullptr, 0);
                        rz_cons_break_pop();
                        rz_core_dbg_follow_seek_register(core);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    }

    emit debugTaskStateChanged();
    connect(debugTask.get(), &RizinTask::finished, this, [this]() {
        debugTask.reset();
        syncAndSeekProgramCounter();
        emit refreshCodeViews();
        emit debugTaskStateChanged();
    });

    debugTask->startTask();
}

void DynamicSession::stepDebug()
{
    if (!currentlyDebugging) {
        return;
    }

    if (currentlyEmulating) {
        if (!asyncTask(
                    [](RzCore *core) {
                        rz_core_esil_step(core, UT64_MAX, nullptr, nullptr, false);
                        rz_core_reg_update_flags(core);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    } else {
        if (!asyncTask(
                    [](RzCore *core) {
                        rz_core_debug_step_one(core, 1);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    }

    emit debugTaskStateChanged();
    connect(debugTask.get(), &RizinTask::finished, this, [this]() {
        debugTask.reset();
        syncAndSeekProgramCounter();
        emit refreshCodeViews();
        emit debugTaskStateChanged();
    });

    debugTask->startTask();
}

void DynamicSession::stepOverDebug()
{
    if (!currentlyDebugging) {
        return;
    }

    if (currentlyEmulating) {
        if (!asyncTask(
                    [&](RzCore *core) {
                        rz_core_analysis_esil_step_over(core);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    } else {
        asyncTask(
                [](RzCore *core) {
                    rz_core_debug_step_over(core, 1);
                    rz_core_dbg_follow_seek_register(core);
                    return nullptr;
                },
                debugTask);
    }

    emit debugTaskStateChanged();
    connect(debugTask.get(), &RizinTask::finished, this, [this]() {
        debugTask.reset();
        syncAndSeekProgramCounter();
        emit refreshCodeViews();
        emit debugTaskStateChanged();
    });

    debugTask->startTask();
}

void DynamicSession::stepOutDebug()
{
    if (!currentlyDebugging) {
        return;
    }

    emit debugTaskStateChanged();
    asyncTask(
            [](RzCore *core) {
                rz_core_debug_step_until_frame(core);
                rz_core_dbg_follow_seek_register(core);
                return nullptr;
            },
            debugTask);

    connect(debugTask.get(), &RizinTask::finished, this, [this]() {
        debugTask.reset();
        syncAndSeekProgramCounter();
        emit refreshCodeViews();
        emit debugTaskStateChanged();
    });

    debugTask->startTask();
}

void DynamicSession::stepBackDebug()
{
    if (!currentlyDebugging) {
        return;
    }

    if (currentlyEmulating) {
        if (!asyncTask(
                    [](RzCore *core) {
                        rz_core_esil_step_back(core);
                        rz_core_reg_update_flags(core);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    } else {
        asyncTask(
                [](RzCore *core) {
                    rz_core_debug_step_back(core, 1);
                    rz_core_dbg_follow_seek_register(core);
                    return nullptr;
                },
                debugTask);
    }
    emit debugTaskStateChanged();

    connect(debugTask.get(), &RizinTask::finished, this, [this]() {
        debugTask.reset();
        syncAndSeekProgramCounter();
        emit refreshCodeViews();
        emit debugTaskStateChanged();
    });

    debugTask->startTask();
}

QStringList DynamicSession::getDebugPlugins()
{
    QStringList plugins;
    auto rizin = lock();
    CutterHtSP<RzDebugPlugin>(rizin.core()->dbg->plugins)
            .ForEach([&plugins](const char * /*k*/, const RzDebugPlugin *plugin) {
                plugins << plugin->name;
                return true;
            });
    return plugins;
}

QString DynamicSession::getActiveDebugPlugin()
{
    auto rizin = lock();
    return rizin->getConfig("dbg.backend");
}

void DynamicSession::setDebugPlugin(const QString &plugin)
{
    auto rizin = lock();
    rizin->setConfig("dbg.backend", plugin);
}

void DynamicSession::startTraceSession()
{
    if (!currentlyDebugging || currentlyTracing) {
        return;
    }

    if (currentlyEmulating) {
        if (!asyncTask(
                    [](RzCore *core) {
                        rz_core_analysis_esil_trace_start(core);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    } else {
        if (!asyncTask(
                    [](RzCore *core) {
                        core->dbg->session = rz_debug_session_new();
                        rz_debug_add_checkpoint(core->dbg);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    }
    emit debugTaskStateChanged();

    connect(debugTask.get(), &RizinTask::finished, this, [this]() {
        delete debugTaskDialog;
        debugTask.reset();

        currentlyTracing = true;
        emit debugTaskStateChanged();
    });

    debugTaskDialog = new RizinTaskDialog(debugTask);
    debugTaskDialog->setBreakOnClose(true);
    debugTaskDialog->setAttribute(Qt::WA_DeleteOnClose);
    debugTaskDialog->setDesc(tr("Creating debug tracepoint..."));
    debugTaskDialog->show();

    debugTask->startTask();
}

bool DynamicSession::isDebugTaskInProgress()
{
    if (debugTask) {
        return true;
    }

    return false;
}

void DynamicSession::stopTraceSession()
{
    if (!currentlyDebugging || !currentlyTracing) {
        return;
    }

    if (currentlyEmulating) {
        if (!asyncTask(
                    [](RzCore *core) {
                        rz_core_analysis_esil_trace_stop(core);
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    } else {
        if (!asyncTask(
                    [](RzCore *core) {
                        rz_debug_session_free(core->dbg->session);
                        core->dbg->session = nullptr;
                        return nullptr;
                    },
                    debugTask)) {
            return;
        }
    }
    emit debugTaskStateChanged();

    connect(debugTask.get(), &RizinTask::finished, this, [this]() {
        delete debugTaskDialog;
        debugTask.reset();

        currentlyTracing = false;
        emit debugTaskStateChanged();
    });

    debugTaskDialog = new RizinTaskDialog(debugTask);
    debugTaskDialog->setBreakOnClose(true);
    debugTaskDialog->setAttribute(Qt::WA_DeleteOnClose);
    debugTaskDialog->setDesc(tr("Stopping debug session..."));
    debugTaskDialog->show();

    debugTask->startTask();
}

bool DynamicSession::isCurrentlyDebugging() const
{
    return currentlyDebugging;
}

bool DynamicSession::isCurrentlyEmulating() const
{
    return currentlyEmulating;
}

bool DynamicSession::isCurrentlyRemoteDebugging() const
{
    return currentlyRemoteDebugging;
}
bool DynamicSession::isCurrentlyTracing() const
{
    return currentlyTracing;
}

bool DynamicSession::isRedirectableDebugee()
{
    if (!currentlyDebugging || currentlyAttachedToPID != -1) {
        return false;
    }
    auto rizin = lock();
    // We are only able to redirect locally debugged unix processes
    return rizin->hasLocalUnixDebuggerIO();
}
