
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

    def init_io(self):
        """Redirect input streams and set a display hook."""
        import sys
        from ipython_genutils.importstring import import_item
        if self.outstream_class:
            print("outstream class {}".format(self.outstream_class))
            outstream_factory = import_item(str(self.outstream_class))
            sys.stdout.flush()

            e_stdout = None if self.quiet else sys.__stdout__
            e_stderr = None if self.quiet else sys.__stderr__

            #sys.stdout = outstream_factory(self.session, self.iopub_thread,
            #                               u'stdout',
            #                               echo=e_stdout)
            sys.stderr.flush()
            #sys.stderr = outstream_factory(self.session, self.iopub_thread,
            #                               u'stderr',
            #                               echo=e_stderr)
        if self.displayhook_class:
            displayhook_factory = import_item(str(self.displayhook_class))
            self.displayhook = displayhook_factory(self.session, self.iopub_socket)
            sys.displayhook = self.displayhook

        self.patch_io()


def launch_ipykernel(argv):
    app = CutterIPKernelApp.instance()

    def run_kernel():
        import asyncio
        asyncio.set_event_loop(asyncio.new_event_loop())
        print("We are on thread {}, NOT SOME DUMMY SHIT!!!!".format(threading.current_thread().name))
        app.kernel_class = CutterIPythonKernel
        # app.log_level = logging.DEBUG
        app.initialize(argv[3:])
        app.start()

    thread = threading.Thread(target=run_kernel)
    thread.start()

    return IPyKernelInterfaceKernel(thread, app)

