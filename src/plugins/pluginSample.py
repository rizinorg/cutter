import cutter
from cutter_plugin import CutterPlugin

class CutterSamplePlugin(CutterPlugin):
    def setupPlugin(self):
        self.name = 'SamplePlugin'
        self.description = 'A sample plugin written in python.'
        self.version = '1.0'
        self.author = 'xarkes'

    def setupInterface(self, main, action):
        print('TODO')


# Instantiate our plugin
plugin = CutterSamplePlugin()

