import sys
import threading
import time
from notebook.notebookapp import *
import cutter


class CutterNotebookApp(NotebookApp):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.thread = None

    def start(self):
        """ see NotebookApp.start() """

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
    print('TODO: Export cutter bindings to any kernel')
    print(cutter.version())
    print(cutter.cmd('?e That is executed from radare2'))
    return app
