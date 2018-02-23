
from jupyter_client.ioloop import IOLoopKernelManager
from notebook.notebookapp import *
import signal
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
        pass


class CutterInternalIPyKernelManager(IOLoopKernelManager):
    def start_kernel(self, **kw):
        # write connection file / get default ports
        self.write_connection_file()

        # save kwargs for use in restart
        self._launch_args = kw.copy()
        # build the Popen cmd
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
        self.log.debug("Starting kernel: %s", kernel_cmd)

        # TODO: kernel_cmd including python executable and so on is currently used for argv. Make a clean version!
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
        super().__init__(**kwargs)
        self.thread = None

    def start(self):
        """ see NotebookApp.start() """

        self.kernel_manager.kernel_manager_factory = kernel_manager_factory

        super(NotebookApp, self).start()

        self.write_server_info_file()

        self.thread = threading.Thread(target=self.run)
        self.thread.start()

    def run(self):
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
        self.thread.join()

    @property
    def url_with_token(self):
        return url_concat(self.connection_url, {'token': self.token})


def start_jupyter():
    app = CutterNotebookApp()
    app.initialize()
    app.start()
    return app


if __name__ == "__main__":
    app = start_jupyter()
    print("Started " + app.url_with_token)
