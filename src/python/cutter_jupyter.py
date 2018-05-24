
import asyncio
import queue
from jupyter_client.ioloop import IOLoopKernelManager
from notebook.notebookapp import *
import cutter_internal


class IPyKernelInterfaceJupyter:
    def __init__(self, id):
        self._id = id

    def send_signal(self, signum):
        cutter_internal.kernel_interface_send_signal(self._id, signum)

    def kill(self):
        self.send_signal(signal.SIGKILL)

    def terminate(self):
        self.send_signal(signal.SIGTERM)

    def poll(self):
        return cutter_internal.kernel_interface_poll(self._id)

    def wait(self, timeout=None):
        if timeout is not None:
            start_time = time.process_time()
        else:
            start_time = None
        while timeout is None or time.process_time() - start_time < timeout:
            if self.poll() is not None:
                return
            time.sleep(0.1)


class CutterInternalIPyKernelManager(IOLoopKernelManager):
    def start_kernel(self, **kw):
        self.write_connection_file()

        self._launch_args = kw.copy()

        extra_arguments = kw.pop('extra_arguments', [])
        kernel_cmd = self.format_kernel_cmd(extra_arguments=extra_arguments)
        env = kw.pop('env', os.environ).copy()
        # Don't allow PYTHONEXECUTABLE to be passed to kernel process.
        # If set, it can bork all the things.
        env.pop('PYTHONEXECUTABLE', None)
        if not self.kernel_cmd:
            # If kernel_cmd has been set manually, don't refer to a kernel spec
            # Environment variables from kernel spec are added to os.environ
            env.update(self.kernel_spec.env or {})

        # launch the kernel subprocess
        id = cutter_internal.launch_ipykernel(kernel_cmd, env=env, **kw)
        self.kernel = IPyKernelInterfaceJupyter(id)
        # self._launch_kernel(kernel_cmd, env=env, **kw)

        self.start_restarter()
        self._connect_control_socket()

    def signal_kernel(self, signum):
        self.kernel.send_signal(signum)


def kernel_manager_factory(kernel_name, **kwargs):
    if kernel_name in {"python", "python2", "python3"}:
        return CutterInternalIPyKernelManager(kernel_name=kernel_name, **kwargs)
    else:
        return IOLoopKernelManager(kernel_name=kernel_name, **kwargs)


class CutterNotebookApp(NotebookApp):
    def __init__(self, **kwargs):
        self.thread = None
        self.io_loop = None
        super().__init__(**kwargs)

    def start(self):
        """ see NotebookApp.start() """
        self.kernel_manager.kernel_manager_factory = kernel_manager_factory

        super(NotebookApp, self).start()

        self.write_server_info_file()

        self.io_loop = ioloop.IOLoop.current()
        if sys.platform.startswith('win'):
            # add no-op to wake every 5s
            # to handle signals that may be ignored by the inner loop
            pc = ioloop.PeriodicCallback(lambda: None, 5000)
            pc.start()
        try:
            self.io_loop.start()
        except KeyboardInterrupt:
            self.log.info(_("Interrupted..."))
        finally:
            self.remove_server_info_file()
            self.cleanup_kernels()

    def stop(self):
        super().stop()
        if self.thread is not None:
            self.thread.join()

    def init_signal(self):
        # This would call signal.signal(signal.SIGINT, signal.SIG_IGN)
        # Not needed in supinterpreter.
        pass

    @property
    def url_with_token(self):
        return url_concat(self.connection_url, {'token': self.token})


def start_jupyter():
    q = queue.Queue()

    def start_jupyter_async():
        asyncio.set_event_loop(asyncio.new_event_loop())
        app = CutterNotebookApp()
        # app.log_level = logging.DEBUG
        app.thread = threading.current_thread()
        app.initialize()
        q.put(app)
        app.start()

    threading.Thread(target=start_jupyter_async).start()
    return q.get()


if __name__ == "__main__":
    app = start_jupyter()
    print("Started " + app.url_with_token)
