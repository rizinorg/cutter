
import logging
import threading
from ipykernel.kernelapp import IPKernelApp
from ipykernel.ipkernel import IPythonKernel


# TODO: Make this behave like a Popen instance and pipe it to IPyKernelInterfaceJupyter!
class IPyKernelInterfaceKernel:
    def poll(self):
        return None


class CutterIPythonKernel(IPythonKernel):
    def pre_handler_hook(self):
        pass

    def post_handler_hook(self):
        pass


class CutterIPKernelApp(IPKernelApp):
    def init_signal(self):
        pass


def launch_ipykernel(argv):
    def run_kernel():
        app = CutterIPKernelApp.instance()
        app.kernel_class = CutterIPythonKernel
        app.log_level = logging.DEBUG
        app.initialize(argv[3:])
        app.start()

    thread = threading.Thread(target=run_kernel)
    thread.start()

    return IPyKernelInterfaceKernel()

