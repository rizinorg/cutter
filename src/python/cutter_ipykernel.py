
import logging
import threading
import signal
import cutter_internal
import zmq
from ipykernel.kernelapp import IPKernelApp
from ipykernel.ipkernel import IPythonKernel


class IPyKernelInterfaceKernel:
    def __init__(self, thread, app):
        self._thread = thread
        self._app = app

    def send_signal(self, signum):
        if not self._thread.is_alive():
            return

        if signum == signal.SIGKILL or signum == signal.SIGTERM:
            self._app.io_loop.stop()
        elif signum == signal.SIGINT and self._app.kernel.interruptable:
            self._app.log.debug("Sending KeyboardInterrupt to ioloop thread.")
            cutter_internal.thread_set_async_exc(self._thread.ident, KeyboardInterrupt())

    def poll(self):
        if self._thread.is_alive():
            return None
        else:
            return 0

    def cleanup(self):
        self._app.heartbeat.context.destroy()
        self._thread.join()
        self._app.heartbeat.join()
        self._app.iopub_thread.stop()
        try:
            self._app.kernel.shell.history_manager.save_thread.stop()
        except AttributeError:
            pass
        zmq.Context.instance().destroy()
        # successful if only the main thread remains
        return len(threading.enumerate()) == 1


class CutterIPythonKernel(IPythonKernel):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.interruptable = False

    def pre_handler_hook(self):
        self.interruptable = True

    def post_handler_hook(self):
        self.interruptable = False


class CutterIPKernelApp(IPKernelApp):
    def init_signal(self):
        # This would call signal.signal(signal.SIGINT, signal.SIG_IGN)
        # Not needed in supinterpreter.
        pass

    def log_connection_info(self):
        # Just skip this. It would only pollute Cutter's output.
        pass


def launch_ipykernel(argv):
    app = CutterIPKernelApp.instance()

    def run_kernel():
        import asyncio
        asyncio.set_event_loop(asyncio.new_event_loop())
        app.kernel_class = CutterIPythonKernel
        # app.log_level = logging.DEBUG
        app.initialize(argv[3:])
        app.start()

    thread = threading.Thread(target=run_kernel)
    thread.start()

    return IPyKernelInterfaceKernel(thread, app)

